/* $Id: title_screen.cpp 48740 2011-03-05 10:01:34Z mordante $ */
/*
   Copyright (C) 2008 - 2011 by Mark de Wever <koraq@xs4all.nl>


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#define GETTEXT_DOMAIN "launcher-lib"

#include "pble2.hpp"

#include <time.h>
#include "game_config.hpp"
// #include "game_preferences.hpp"
#include "gettext.hpp"
#include "help.hpp"
#include "filesystem.hpp"
#include <hero.hpp>
#include "sound.hpp"
#include "wml_exception.hpp"

#include <iomanip>
using namespace std::placeholders;

#include <algorithm>

#include "base_instance.hpp"
#include <kosapi/net.h>

const char* tpble2::uuid_my_service = "fd00";
const char* tpble2::uuid_write_characteristic = "fd01";
const char* tpble2::uuid_notify_characteristic = "fd03";

uint32_t getipv4_prefixlen(const std::string& iface, int* prefixlen)
{
	char result[128];
    const int maxBytes = sizeof(result);
	char msg[128];

	sprintf(msg, "interface getcfg %s", iface.c_str());
    kosNetSendMsg(msg, result, maxBytes);
	if (game_config::os == os_windows) {
		// 36:56:fb:9f:f8:a2 192.168.1.108 24 up broadcast running multicast
		// 00:00:00:00:00:00 0.0.0.0 0 down
		strcpy(result, "36:56:fb:9f:f8:a2 192.168.1.105 24 up broadcast running multicast");
	}
    std::vector<std::string> vstr = utils::split(result, ' ');
    uint32_t ipv4 = 0;
	// net::IPAddress
    if (vstr.size() >= 4) {
        ipv4 = utils::to_ipv4(vstr[1]);
		if (prefixlen != nullptr) {
			*prefixlen = utils::to_int(vstr[2]);
		}
    }
	return ipv4;
}

tpble2::tpble2(base_instance* instance)
	: tpble(instance)
	, status_(nposm)
	, buf_(false)
{
	buf_.set_did_read(std::bind(&tpble2::did_read_ble, this, _1, _2, _3));
}

tpble2::~tpble2()
{
}

void tpble2::app_center_connected(const tmac_addr& center, int error)
{
	if (error == 0) {
		std::pair<std::map<std::string, tsendbuf>::iterator, bool> ins = send_bufs_.insert(std::make_pair(uuid_notify_characteristic, tsendbuf()));
		ins.first->second.set_did_write(std::bind(&tpble::write_characteristic, this, uuid_notify_characteristic, _1, _2));
	}
}

void tpble2::app_center_disconnected(const tmac_addr& center)
{
	set_status(nposm);
}

void tpble2::app_read_characteristic(const std::string& characteristic, const unsigned char* data, int len)
{
	SDL_Log("tpble2::app_read_characteristic(%s)--- len: %i", characteristic.c_str(), len);
	if (SDL_BleUuidEqual(characteristic.c_str(), uuid_write_characteristic)) {
		buf_.enqueue(data, len);
	}
}

void tpble2::form_wifilist_resp_ipv4_ex(bool force, tuint8data& out)
{
	memset(&out, 0, sizeof(out));

	uint32_t wifi_flags = SDL_WifiGetFlags();
	if (force || (wifi_flags & SDL_WifiFlagScanResultChanged)) {
		SDL_WifiScanResult* results = nullptr;
		int count = SDL_WifiGetScanResults(&results);
		uint32_t flags = 0;
		if (wifi_flags & SDL_WifiFlagEnable) {
			flags |= LEAGOR_BLE_FLAG_WIFI_ENABLED;
		}
		buf_.form_wifilist_resp_ipv4(instance->current_ip(), flags, results, count, out);
	}
}

void tpble2::did_read_ble(int cmd, const uint8_t* data, int len)
{
	std::string str = rtc::hex_encode((const char*)data, len);
	SDL_Log("%u tpble2::did_read_ble, %s", SDL_GetTicks(), str.c_str());

	if (cmd == msg_queryip_req) {
		set_status(status_queryip);

		uint32_t wifi_flags = SDL_WifiGetFlags();
		uint32_t flags = 0;
		if (wifi_flags & SDL_WifiFlagEnable) {
			flags |= LEAGOR_BLE_FLAG_WIFI_ENABLED;
		}

		tblebuf::tqueryip_resp src;
		memset(&src, 0, sizeof(src));
		src.version = LEAGOR_BLE_VERSION;
		src.flags = flags;

		tuint8data resp_result;
		int client_version = buf_.parse_queryip_req(data, len);
		if (client_version == LEAGOR_BLE_VERSION) {
			uint32_t ip = instance->current_ip();
			int prefixlen = 0;
			uint32_t ip2 = getipv4_prefixlen("eth0", &prefixlen);
			if (ip != ip2) {
				prefixlen = 0;
			}

			src.af = LEAGOR_BLE_AF_INET;
			src.a.ipv4 = instance->current_ip();
			
		} else {
			SDL_Log("msg_queryip_req, client(%i) != server(%i), dismatch version", client_version, LEAGOR_BLE_VERSION);
			src.af = LEAGOR_BLE_AF_UNSPEC;
		}
		buf_.form_queryip_resp(src, resp_result);

		SDL_Log("receive request: query ip");
		send_bufs_.find(uuid_notify_characteristic)->second.enqueue(resp_result.ptr, resp_result.len);
		// write_characteristic(uuid_notify_characteristic, data, data_len);

	} else if (cmd == msg_updateip_req) {
		set_status(status_updateip);

		SDL_WifiSetEnable(SDL_TRUE);
		tuint8data data;
		form_wifilist_resp_ipv4_ex(true, data);

		send_bufs_.find(uuid_notify_characteristic)->second.enqueue(data.ptr, data.len);

		// write_characteristic(uuid_notify_characteristic, data, data_len);

	} else if (cmd == msg_connectwifi_req) {
		SDL_Log("receive request: connect wifi, len: %i, status: %i", len, status_);
		if (status_ == status_updateip) {
			std::string password;
			const std::string ssid = buf_.parse_connectwifi_req(data, len, password);

			if (!ssid.empty() && !password.empty()) {
				SDL_WifiConnect(ssid.c_str(), password.c_str());
			}
		}

	} else if (cmd == msg_removewifi_req) {
		SDL_Log("receive request: remove wifi, len: %i, status: %i", len, status_);
		if (status_ == status_updateip) {
			const std::string ssid = buf_.parse_removewifi_req(data, len);

			if (!ssid.empty()) {
				SDL_WifiRemove(ssid.c_str());
			}
		}

	} else if (cmd == msg_wifilist_req) {
		if (status_ == status_updateip) {
			tuint8data data;
			form_wifilist_resp_ipv4_ex(true, data);

			send_bufs_.find(uuid_notify_characteristic)->second.enqueue(data.ptr, data.len);
		}
	}
}

void tpble2::app_notification_sent(const std::string& characteristic, int error)
{
	SDL_Log("tpble2::app_notification_sent(%s)--- error: %i", characteristic.c_str(), error);
	if (SDL_BleUuidEqual(characteristic.c_str(), uuid_notify_characteristic)) {
		send_bufs_.find(uuid_notify_characteristic)->second.did_notification_sent(error);
	}
}

void tpble2::simulate_read_characteristic()
{
	VALIDATE(game_config::os == os_windows, null_str);

	uint8_t resp[2];
	resp[0] = 0x07;
	resp[1] = 0x01;

/*
	uint8_t resp[11];
	resp[0] = 0x07;
	resp[1] = 0x03;
	uint32_t ipv4 = 0x7401a8c0; // 192.168.1.116
	memcpy(resp + 2, &ipv4, 4);
	resp[6] = 24;
	uint32_t gateway = 0x0101a8c0; // 192.168.1.1
	memcpy(resp + 7, &gateway, 4);
*/
	app_read_characteristic(uuid_write_characteristic, resp, sizeof(resp));
}

void tpble2::timer_handler(uint32_t now)
{
	if (status_ == status_updateip) {
		tuint8data data;
		form_wifilist_resp_ipv4_ex(false, data);
		if (data.len > 0) {
			send_bufs_.find(uuid_notify_characteristic)->second.enqueue(data.ptr, data.len);
		}
	}
}