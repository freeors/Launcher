/* Require Rose v1.0.19 or above. $ */

#define GETTEXT_DOMAIN "launcher-lib"

#include "rdp_server_rose.h"
#include "base_instance.hpp"
#include "gui/dialogs/message.hpp"
#include "gui/dialogs/chat.hpp"
#include "gui/dialogs/home.hpp"
#include "gui/dialogs/explorer.hpp"
#include "gui/dialogs/settings.hpp"
#include "gui/widgets/window.hpp"
#include "game_end_exceptions.hpp"
#include "wml_exception.hpp"
#include "gettext.hpp"
#include "loadscreen.hpp"
#include "formula_string_utils.hpp"
#include "help.hpp"
#include "version.hpp"
#include <kosapi/sys.h>
#include <kosapi/net.h>

#include "game_config.hpp"
#include "pble2.hpp"
#include "ResponseCode.h"

extern uint32_t ReceivingInterfaceAddr;

class game_instance: public base_instance
{
public:
	game_instance(rtc::PhysicalSocketServer& ss, int argc, char** argv);
	~game_instance();

	void did_receive_broadcast(int argc, const char** argv);
	void did_net_state_changed(const char* iface, bool connected, uint32_t ipv4);

	net::trdpd_manager& rdpd_mgr() { return rdpd_mgr_; }
	tpble2& pble() { return pble_; }

	void test_did_receive_broadcast();

private:
	void app_load_settings_config(const config& cfg) override;
	void app_pre_setmode(tpre_setmode_settings& settings) override;
	void app_load_pb() override;
	config app_critical_prefs() override;
	void app_handle_clipboard_paste(const std::string& text) override;
	bool app_update_ip(const uint32_t ipv4, const int prefixlen, const uint32_t gateway) override;
	void app_didenterbackground() override;
	void app_didenterforeground() override;

private:
	net::trdpd_manager rdpd_mgr_;
	tpble2 pble_;
};

static void did_net_receive_broadcast(int argc, const char** argv, void* user)
{
	game_instance* instance = reinterpret_cast<game_instance*>(user);
	instance->did_receive_broadcast(argc, argv);
}

void game_instance::did_receive_broadcast(int argc, const char** argv)
{
	if (argc <= 1) {
		return;
	}
	int code = utils::to_int(argv[0]);
	if (code == ResponseCode::InterfaceAddressChange) {
		// 614    Address updated         192.168.1.108/24             wlan0   128   0
		// 614    Address removed         192.168.1.108/24             wlan0   128   0
		// 614    Address updated/removed 192.168.1.109/24             eth0    128    0
		// 614    Address updated         fe80::ce4b:73ff:fe1d:7f1c/64 wlan0   196    253
		// 614    Address removed         fe80::ce4b:73ff:fe1d:7f1c/64 wlan0   128    253
        // [code] [name]                  [address]                    [iface] [flag] [scope]
		const char *kUpdated = "updated";
		const char *kRemoved = "removed";
        if (argc != 7) {
            return;
        }
        if (strcmp(argv[1], "Address")) {
            return;
        }
        bool updated = true;

        if (!strcmp(argv[2], kUpdated)) {
            // Address updated
            updated = true;

        } else if (!strcmp(argv[2], kRemoved)) {
            // Address removed
            updated = false;
        } else {
            return;
        }

		net::IPAddress ip_address;
		size_t prefix_length_in_bits;
		bool ok = net::ParseCIDRBlock(argv[3], &ip_address, &prefix_length_in_bits);
		if (ok) {
			if (ip_address.IsIPv4()) {
				const uint8_t* data = ip_address.bytes().data();
				uint32_t ipv4;
				memcpy(&ipv4, data, 4);
				did_net_state_changed(argv[4], updated, ipv4);
			} else if (ip_address.IsIPv6()) {
			}
		}
	}
}

void game_instance::did_net_state_changed(const char* iface, bool connected, uint32_t ipv4)
{
	SDL_Log("did_net_state_changed(iface: %s, connected: %s, ipv4: 0x%08x) ip: 0x%08x",
		iface, connected? "true": "false", ipv4, ReceivingInterfaceAddr);

	if (!connected && ReceivingInterfaceAddr != INADDR_ANY && ipv4 == ReceivingInterfaceAddr) {
		// run not in main thread.
		make_ip_invalid();
	}
}

void game_instance::test_did_receive_broadcast()
{
	VALIDATE(game_config::os == os_windows, null_str);
	std::vector<std::string> argvs;
	argvs.push_back("612, Address, updated, 192.168.1.100/24, wlan0, 128, 0");
	argvs.push_back("614, Address, updated, 192.168.1.100/24, wlan0, 128");
	argvs.push_back("614, Address, updated, 192.168.1.100/24, wlan0, 128, 0");
	argvs.push_back("614, Address, removed, 192.168.1.100/24, wlan0, 128, 0");
	argvs.push_back("614, Address, updated/removed, 192.168.1.109/24, eth0, 128, 0");
	argvs.push_back("614, Address, updated, fe80::ce4b:73ff:fe1d:7f1c/64, wlan0, 196, 253");

	char* argv[24];
	for (std::vector<std::string>::const_iterator it = argvs.begin(); it != argvs.end(); it ++) {
		const std::string& msg = *it;
		std::vector<std::string> vstr = utils::split(msg);
		int n = 0;
		for (std::vector<std::string>::const_iterator it2 = vstr.begin(); it2 != vstr.end(); ++ it2, n ++) {
			const std::string& field = *it2;
			argv[n] = (char*)malloc(field.size() + 1);
			memcpy(argv[n], field.c_str(), field.size());
			argv[n][field.size()] = '\0';
		}
		int argc = n;
		did_net_receive_broadcast(argc, (const char**)argv, this);
		for (n = 0; n < argc; n ++) {
			free(argv[n]);
		}
	}
	SDL_Log("game_instance::test_did_receive_broadcast X");
}

game_instance::game_instance(rtc::PhysicalSocketServer& ss, int argc, char** argv)
	: base_instance(ss, argc, argv)
	, pble_(this)
{
	kosNetSetReceiveBroadcast(::did_net_receive_broadcast, this);
}

game_instance::~game_instance()
{
}

void game_instance::app_load_settings_config(const config& cfg)
{
	game_config::version = version_info(cfg["version"].str());
	VALIDATE(game_config::version.is_rose_recommended(), null_str);

	char libkosapi_ver[36];
	kosGetVersion(libkosapi_ver, sizeof(libkosapi_ver));
	game_config::kosapi_ver = version_info(libkosapi_ver);
	VALIDATE(game_config::kosapi_ver.is_rose_recommended(), std::string("Error version: ") + game_config::kosapi_ver.str(true));
	const version_info min_libkosapi_ver("1.0.3-20210422");
	if (game_config::kosapi_ver < min_libkosapi_ver) {
		std::stringstream err;
		err << "libkospai's version(" << game_config::kosapi_ver.str(true) << ") must >= " << min_libkosapi_ver.str(true);
		VALIDATE(false, err.str());
	}

	game_config::max_fps_to_encoder = cfg["max_fps_to_encoder"].to_int();
	VALIDATE(game_config::max_fps_to_encoder == 0 || (game_config::max_fps_to_encoder >= 20 && game_config::max_fps_to_encoder <= 60), null_str);

	// if any key in critical prefs isn't in preferences, try read it from critical_prefs
	{
		const config& critical_prefs = preferences::get_critical_prefs();

		const std::string sn = critical_prefs["sn"].str();
		const bool bleperipheral = critical_prefs["bleperipheral"].to_bool();
		
		// SDL_Log("app_load_settings_config, dump critical_prefs");
		// SDL_Log("    sn: %s", sn.c_str());
		// SDL_Log("    bleperipheral: %s", bleperipheral? "true": "false");

		if (preferences::bleperipheral() != bleperipheral) {
			preferences::set_bleperipheral(bleperipheral);
		}

		if (preferences::sn().empty() && !sn.empty()) {
			preferences::set_sn(sn);
		}
	}
}

void game_instance::app_pre_setmode(tpre_setmode_settings& settings)
{	
	settings.default_font_size = 18;
	settings.fullscreen = false;
	// settings.silent_background = false;
	if (game_config::os == os_windows) {
		settings.min_width = 640;
		settings.min_height = 360;
	}
	// settings.startup_servers = server_httpd;
}

void game_instance::app_load_pb()
{
	utils::string_map symbols;
	symbols["count"] = "1";
	game_config::suppress_thresholds.insert(std::make_pair(1 * 60, vgettext2("$count minutes", symbols)));
	symbols["count"] = "15";
	game_config::suppress_thresholds.insert(std::make_pair(15 * 60, vgettext2("$count minutes", symbols)));
	symbols["count"] = "30";
	game_config::suppress_thresholds.insert(std::make_pair(30 * 60, vgettext2("$count minutes", symbols)));
	symbols["count"] = "1";
	game_config::suppress_thresholds.insert(std::make_pair(1 * 3600, vgettext2("$count hours", symbols)));
	game_config::suppress_thresholds.insert(std::make_pair(nposm, _("No restrictions")));
	VALIDATE(game_config::suppress_thresholds.count(DEFAULT_SUPRESS_THRESHOLD) != 0, null_str);
}

config game_instance::app_critical_prefs()
{
	config cfg;

	const std::string sn = preferences::sn();
	if (sn.empty()) {
		return cfg;
	}
	cfg["sn"] = sn;
	cfg["bleperipheral"] = preferences::bleperipheral();

	return cfg;
}

void game_instance::app_handle_clipboard_paste(const std::string& text)
{
	rdpd_mgr_.clipboard_updated(text);
}

uint32_t calculate_widecard_ip(uint32_t ipv4, int prefixlen)
{
	VALIDATE(prefixlen >= 1 && prefixlen < 32, null_str);
	uint32_t ret = SDL_Swap32(ipv4) & ~((1u << (32 - prefixlen)) - 1);
	return SDL_Swap32(ret);
}

bool game_instance::app_update_ip(const uint32_t ipv4, const int prefixlen, const uint32_t _gateway)
{
	VALIDATE(ipv4 != 0, null_str);
	VALIDATE(prefixlen >= 1 && prefixlen < 32, null_str);
	VALIDATE(_gateway != 0, null_str);

	const std::string iface = "eth0";

	net::IPAddress ipaddr((const uint8_t*)&ipv4, 4);
	net::IPAddress gateway((const uint8_t*)&_gateway, 4);
    char result[128];
    const int maxBytes = sizeof(result);
	std::unique_ptr<char> msg(new char[1024]);
    char* msg_ptr = msg.get();

	sprintf(msg_ptr, "launcher netid %s", iface.c_str());
    int code = kosNetSendMsg(msg_ptr, result, maxBytes);
	if (game_config::os == os_windows) {
		SDL_snprintf(result, maxBytes, "%s 100", iface.c_str());
	}
    std::vector<std::string> vstr = utils::split(result, ' ');
    int netId = nposm;
    if (vstr.size() == 2 && vstr[0] == iface) {
         netId = utils::to_int(vstr[1]);
    }
    if (netId < 0) {
        SDL_Log("app_update_ip, result: %s, len: %i, netId(%i) fail", result, (int)strlen(result), netId);
        return false;
    }

	// interface getcfg eth0
	sprintf(msg_ptr, "interface getcfg %s", iface.c_str());
    kosNetSendMsg(msg_ptr, result, maxBytes);
	if (game_config::os == os_windows) {
		// 36:56:fb:9f:f8:a2 192.168.1.108 24 up broadcast running multicast
		// 00:00:00:00:00:00 0.0.0.0 0 down
		strcpy(result, "36:56:fb:9f:f8:a2 192.168.1.108 24 up broadcast running multicast");
	}
    vstr = utils::split(result, ' ');
    uint32_t originalIpv4 = 0;
	// net::IPAddress
    if (vstr.size() >= 4) {
        originalIpv4 = utils::to_ipv4(vstr[1]);
    }

    kosNetSendMsg("launcher broadcast disable", result, maxBytes);
	// interface clearaddrs eth0
	sprintf(msg_ptr, "interface clearaddrs %s", iface.c_str());
    kosNetSendMsg(msg_ptr, result, maxBytes);

	did_net_state_changed(nullptr, false, originalIpv4);

	// interface setcfg eth0 192.168.1.116 24 multicast up broadcast running
	sprintf(msg_ptr, "interface setcfg %s %s %i multicast up broadcast running", iface.c_str(), ipaddr.ToString().c_str(), prefixlen);
    kosNetSendMsg(msg_ptr, result, maxBytes);

	// network route add 100 eth0 192.168.1.0/24
	uint32_t widecard_gateway = calculate_widecard_ip(_gateway, prefixlen);
	net::IPAddress widecard_gateway2((const uint8_t*)&widecard_gateway, 4);
    sprintf(msg_ptr, "network route add %i %s %s/%i", netId, iface.c_str(), widecard_gateway2.ToString().c_str(), prefixlen);
    kosNetSendMsg(msg_ptr, result, maxBytes);

	// network route add 100 eth0 0.0.0.0/0 192.168.1.1
    sprintf(msg_ptr, "network route add %i %s 0.0.0.0/0 %s", netId, iface.c_str(), gateway.ToString().c_str());
    kosNetSendMsg(msg_ptr, result, maxBytes);

	// network default set 100
    kosNetSendMsg("launcher broadcast enable", result, maxBytes);

	return true;
}

void game_instance::app_didenterbackground()
{
	if (game_config::explorer_singleton != nullptr) {
		gui2::texplorer_slot* slot = reinterpret_cast<gui2::texplorer_slot*>(game_config::explorer_singleton);
		slot->send_LG_EXPLORER_CODE_HIDDEN();
	}
}

void game_instance::app_didenterforeground()
{
	if (game_config::explorer_singleton != nullptr) {
		gui2::texplorer_slot* slot = reinterpret_cast<gui2::texplorer_slot*>(game_config::explorer_singleton);
		slot->send_LG_EXPLORER_CODE_SHOWN();
	}
}

/**
 * Setups the game environment and enters
 * the titlescreen or game loops.
 */
static int do_gameloop(int argc, char** argv)
{
	SDL_SetHint(SDL_HINT_BLE, "1");

	rtc::PhysicalSocketServer ss;
	instance_manager<game_instance> manager(ss, argc, argv, "launcher", "#rose");
	game_instance& game = manager.get();

	try {
		preferences::set_use_rose_keyboard(false);

		game.register_server(server_rdpd, &game.rdpd_mgr());

		for (; ;) {
			game.loadscreen_manager().reset();
			const font::floating_label_context label_manager;
			cursor::set(cursor::NORMAL);

			int res;
			{
				gui2::thome dlg(game.rdpd_mgr(), game.pble());
				dlg.show();
				res = static_cast<gui2::thome::tresult>(dlg.get_retval());
			}

			if (res == gui2::thome::EXPLORER) {
				if (game_config::os == os_windows) {
					// game.test_did_receive_broadcast();
				}

				SDL_Rect win_rect;
				SDL_Window* sdl_window = get_sdl_window();
				if (game_config::os == os_windows) {
					SDL_GetWindowPosition(sdl_window, &win_rect.x, &win_rect.y);
					SDL_GetWindowSize(sdl_window, &win_rect.w, &win_rect.h);
					// SDL_SetWindowResizable(sdl_window, SDL_FALSE);
					// SDL_SetWindowBordered(get_sdl_window(), SDL_FALSE);
					SDL_MaximizeWindow(sdl_window);
				}
				gui2::trexplorer::tentry extra(null_str, null_str, null_str);
				if (game_config::os == os_windows) {
					extra = gui2::trexplorer::tentry(game_config::path + "/data/gui/default/scene", _("gui/scene"), "misc/dir_res.png");
				} else if (game_config::os == os_android) {
					extra = gui2::trexplorer::tentry("/sdcard", "/sdcard", "misc/dir_res.png");
				}

				gui2::texplorer_slot slot(game.rdpd_mgr(), game.pble());
				bool click_open_dir = game.rdpd_mgr().started()? game.rdpd_mgr().client_is_mobile(): game_config::mobile;
				// When ios client connect to the server, server may be running trexplorer, 
				// and this is opened by the previous windows client, which will cause ios confusion. 
				// It's better to use unified rules for the all os.
				click_open_dir = true;
				gui2::trexplorer dlg(slot, null_str, extra, false, click_open_dir);
				dlg.show();
				int res = dlg.get_retval();
				if (game_config::os == os_windows) {
					// above SDL_MaximizeWindow myabe result recreate SDL_window, so get again.
					// sdl_window = get_sdl_window();
/*
					// SDL_SetWindowBordered(get_sdl_window(), SDL_FALSE);
					SDL_SetWindowPosition(sdl_window, win_rect.x, win_rect.y);
					// SDL_SetWindowResizable(sdl_window, SDL_TRUE);
					SDL_SetWindowSize(sdl_window, win_rect.w, win_rect.h);
*/
					// if use SDL_SetWindowPosition+SDL_SetWindowSize, maybe cann't clear SDL_WINDOW_MAXIMIZED,
					// it will result next SDL_MaximizeWindow do nothing.
					SDL_RestoreWindow(sdl_window);
				}
				if (res != gui2::twindow::OK) {
					continue;
				}

			} else if (res == gui2::thome::SETTINGS) {
				gui2::tsettings dlg(game.rdpd_mgr(), game.pble());
				dlg.show();
			}

		}

	} catch (twml_exception& e) {
		e.show();

	} catch (CVideo::quit&) {
		//just means the game should quit
		SDL_Log("SDL_main, catched CVideo::quit\n");

	} catch (game_logic::formula_error& e) {
		gui2::show_error_message(e.what());
	} 

	return 0;
}

int main(int argc, char** argv)
{
	try {
		do_gameloop(argc, argv);
	} catch (twml_exception& e) {
		// this exception is generated when create instance.
		e.show();
	}

	return 0;
}