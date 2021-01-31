#define GETTEXT_DOMAIN "launcher-lib"

#include "gui/dialogs/settings.hpp"

#include "gui/widgets/label.hpp"
#include "gui/widgets/button.hpp"
#include "gui/widgets/toggle_button.hpp"
#include "gui/widgets/window.hpp"
#include "gui/dialogs/menu.hpp"
#include "gettext.hpp"
#include "formula_string_utils.hpp"
#include "filesystem.hpp"
#include "version.hpp"
#include "base_instance.hpp"

#include "game_config.hpp"

using namespace std::placeholders;

#include <kosapi/net.h>

namespace gui2 {

REGISTER_DIALOG(launcher, settings)

tsettings::tsettings(net::trdpd_manager& rdpd_mgr, tpble2& pble)
	: tstatusbar(rdpd_mgr, pble)
{
	set_timer_interval(800);
}

void tsettings::pre_show()
{
	window_->set_escape_disabled(true);
	window_->set_label("misc/white_background.png");

	tstatusbar::pre_show(*window_, find_widget<tgrid>(window_, "statusbar_grid", false));

	// find_widget<tbutton>(window_, "cancel", false).set_icon("misc/back.png");

	tbutton* button = find_widget<tbutton>(window_, "getcfg", false, true);
	connect_signal_mouse_left_click(
			  *button
			, std::bind(
			&tsettings::click_getcfg
			, this, std::ref(*button)));

	button = find_widget<tbutton>(window_, "setcfg", false, true);
	connect_signal_mouse_left_click(
			  *button
			, std::bind(
			&tsettings::click_setcfg
			, this, std::ref(*button)));
}

void tsettings::post_show()
{
}

void tsettings::click_getcfg(tbutton& widget)
{
	pble_.simulate_read_characteristic();
}

void tsettings::click_setcfg(tbutton& widget)
{
	// 192.168.1.116 ==> 0x7401a8c0

	uint32_t ipv4 = 0x7001a8c0;
	int prefixlen = 24;
	uint32_t gateway = 0x0101a8c0;
	instance->app_update_ip(ipv4, prefixlen, gateway);
}


void tsettings::app_timer_handler(uint32_t now)
{
	refresh_statusbar_grid(now);
}

} // namespace gui2

