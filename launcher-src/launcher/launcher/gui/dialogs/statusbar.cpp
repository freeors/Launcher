#define GETTEXT_DOMAIN "launcher-lib"

#include "gui/dialogs/statusbar.hpp"

#include "gui/widgets/label.hpp"
#include "gui/widgets/image.hpp"
#include "gui/widgets/window.hpp"
#include "gettext.hpp"
#include "formula_string_utils.hpp"
#include "game_config.hpp"
#include "filesystem.hpp"
#include "SDL_power.h"
#include "version.hpp"

#include <net/server/rdp_connection.h>

namespace gui2 {

tstatusbar::tstatusbar(net::trdpd_manager& rdpd_mgr, tpble2& pble)
	: rdpd_mgr_(rdpd_mgr)
	, pble_(pble)
	, window_priv_(nullptr)
	, statusbar_widget_(nullptr)
	, ble_icon_widget_(nullptr)
	, client_icon_widget_(nullptr)
	, client_ip_widget_(nullptr)
	, time_widget_(nullptr)
	, next_ble_ticks_(0)
{
}

void tstatusbar::pre_show(twindow& window, tgrid& statusbar_widget)
{
	VALIDATE(!window_priv_ && !statusbar_widget_, null_str);
	window_priv_ = &window;
	statusbar_widget_ = &statusbar_widget;

	ble_icon_widget_ = find_widget<timage>(&window, "bluetooth_icon", false, true);
	client_icon_widget_ = find_widget<timage>(&window, "client_icon", false, true);
	client_ip_widget_ = find_widget<tlabel>(&window, "client_ip", false, true);
	time_widget_ = find_widget<tlabel>(&window, "time", false, true);

	refresh_statusbar_grid(SDL_GetTicks());
}

void tstatusbar::refresh_statusbar_grid(uint32_t now)
{
	pble_.timer_handler(now);

	// ble section
	std::string ble_icon = "misc/bluetooth.png";
	if (pble_.is_connected()) {
		ble_icon = "misc/bluetooth-connected.png";
	} else if (pble_.is_advertising()) {
		if (now >= next_ble_ticks_) {
			if (!current_ble_icon_.empty()) {
				ble_icon.clear();
			}
			const int blink_threshold = 100;
			next_ble_ticks_ = now + blink_threshold;
		} else {
			ble_icon = current_ble_icon_;
		}
	}
	ble_icon_widget_->set_label(ble_icon);
	current_ble_icon_ = ble_icon;

	// client section
	std::string client_icon = "misc/no-mobile.png";
	std::stringstream client_ip;
	if (rdpd_mgr_.started()) {
		net::RdpServer& server = rdpd_mgr_.rdp_server();
		if (server.normal_connection_count() != 0) {
			client_icon = "misc/mobile.png";
			net::RdpConnection* connection = server.FindFirstNormalConnection();
			VALIDATE(connection != nullptr, null_str);
			const net::IPEndPoint& peer = connection->peer_ip();
			client_ip << peer.ToStringWithoutPort();
			if (!connection->handshaked()) {
				client_ip << "(" << _("Unhandshakse") << ")";
			}
		}
	}
	client_icon_widget_->set_label(client_icon);
	client_ip_widget_->set_label(client_ip.str());

	std::stringstream ss;
	ss << rdpd_mgr_.url();
	time_widget_->set_label(ss.str());
}

} // namespace gui2

