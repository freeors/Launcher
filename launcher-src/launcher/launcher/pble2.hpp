#ifndef PBLE2_HPP_INCLUDED
#define PBLE2_HPP_INCLUDED

#include <set>
#include <vector>
#include <map>
#include <filesystem.hpp>
#include "ble.hpp"
#include "thread.hpp"

#include "freerdp/channels/leagor_ble.hpp"


class tpble2: public tpble
{
public:

	static const char* uuid_my_service;
	static const char* uuid_write_characteristic;
	static const char* uuid_notify_characteristic;

	tpble2(base_instance* instance);
	~tpble2();

	void set_status(int status) { status_ = status; }
	void timer_handler(uint32_t now);

	void simulate_read_characteristic();
private:
	void app_center_connected(const tmac_addr& center, int error) override;
	void app_center_disconnected(const tmac_addr& center) override;
	void app_read_characteristic(const std::string& characteristic, const unsigned char* data, int len) override;
	void app_notification_sent(const std::string& characteristic, int error) override;
	void did_read_ble(int cmd, const uint8_t* data, int len);

	void form_wifilist_resp_ipv4_ex(bool force, tuint8data& out);

private:
	enum {status_queryip, status_updateip};
	int status_;
	tblebuf buf_;
};

#endif
