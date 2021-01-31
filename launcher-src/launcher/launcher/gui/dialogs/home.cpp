#define GETTEXT_DOMAIN "launcher-lib"

#include "gui/dialogs/home.hpp"

#include "gui/widgets/label.hpp"
#include "gui/widgets/button.hpp"
#include "gui/widgets/toggle_button.hpp"
#include "gui/widgets/window.hpp"
#include "gui/dialogs/menu.hpp"
#include "gui/dialogs/edit_box.hpp"
#include "gettext.hpp"
#include "formula_string_utils.hpp"
#include "filesystem.hpp"
#include "version.hpp"
#include "base_instance.hpp"

#include "game_config.hpp"

using namespace std::placeholders;

#define MIN_SN_CHARS	5
// On Adnroid, 'name' + 'manufacturerdata' up to 19 bytes. 'name' fixed is 4 bytes (rdpd), 
// so 'manufacturerdata' can be up to 15 bytes, but for safe, set to 13 bytes currently.
#define MAX_SN_CHARS	13
#define PERIPHERAL_NAME	"rdpd"

namespace gui2 {

REGISTER_DIALOG(launcher, home)

thome::thome(net::trdpd_manager& rdpd_mgr, tpble2& pble)
	: tstatusbar(rdpd_mgr, pble)
	, peripheral_name_(PERIPHERAL_NAME)
	, manufacturer_id_(65520) // 0xfff0
	, start_ticks_(SDL_GetTicks())
	, status_widget_(nullptr)
{
	set_timer_interval(800);
}

void thome::pre_show()
{
	window_->set_escape_disabled(true);
	window_->set_label("misc/white_background.png");

	tstatusbar::pre_show(*window_, find_widget<tgrid>(window_, "statusbar_grid", false));

	std::stringstream ss;
	utils::string_map symbols;

	symbols["explorer"] = _("icon^Explorer");
	ss.str("");
	ss << vgettext2("launcher user remark, $explorer", symbols);
	ss << "\n";
	ss << "V" << game_config::version.str(true);
	ss << "     libkosapi: V" << game_config::kosapi_ver.str(true);
	find_widget<tlabel>(window_, "version", false).set_label(ss.str());

	tbutton* button = find_widget<tbutton>(window_, "explorer", false, true);
	connect_signal_mouse_left_click(
			  *button
			, std::bind(
			&thome::click_explorer
			, this));

	button = find_widget<tbutton>(window_, "settings", false, true);
	connect_signal_mouse_left_click(
			  *button
			, std::bind(
			&thome::click_settings
			, this, std::ref(*button)));
	button->set_visible(twidget::INVISIBLE);
	find_widget<tlabel>(window_, "settings_label", false, true)->set_visible(twidget::INVISIBLE);

	status_widget_ = find_widget<tlabel>(window_, "status", false, true);
	status_widget_->set_visible(twidget::INVISIBLE);

	find_widget<tlabel>(window_, "sn_label", false).set_label(_("Device's SN in BLE Discovery"));
	button = find_widget<tbutton>(window_, "update_sn", false, true);
	connect_signal_mouse_left_click(
			  *button
			, std::bind(
			&thome::click_update_sn
			, this, std::ref(*button)));
	button->set_label(preferences::sn());

	// enable/disable bleperipheral
	ttoggle_button* toggle = find_widget<ttoggle_button>(window_, "bleperipheral", false, true);
	toggle->set_did_state_changed(std::bind(&thome::did_bleperipheral_changed, this, _1));
	toggle->set_value(preferences::bleperipheral());
}

void thome::post_show()
{
}

void thome::click_explorer()
{
	window_->set_retval(EXPLORER);

	// SDL_UpdateApp("com.leagor.studio");
}

void thome::click_settings(tbutton& widget)
{
	window_->set_retval(SETTINGS);
}

void thome::click_aismart()
{
/*
	// SDL_UpdateApp("com.leagor.aismart");
*/
}

void thome::did_bleperipheral_changed(ttoggle_button& widget)
{
	if (widget.get_value()) {
		// disable --> enable
		if (!pble_.is_advertising()) {
			start_advertising();
		}

	} else {
		// enable --> disable
		if (pble_.is_advertising()) {
			pble_.stop_advertising();
		}
	}
	preferences::set_bleperipheral(widget.get_value());
}

static bool is_valid_sn_char(uint8_t ch)
{
	if ((ch & 0x80) || ch <= 0x20 || ch == 0x7f) {
		return false;
	}
	return true;
}

bool thome::did_verify_sn(const std::string& label)
{
	const int size = label.size();
	if (size < MIN_SN_CHARS || size > MAX_SN_CHARS) {
		return false;
	}
	if (label == preferences::sn()) {
		return false;
	}
	const char* c_str = label.c_str();
	for (int at = 0; at < size; at ++) {
		uint8_t ch = c_str[at];
		if (!is_valid_sn_char(ch)) {
			return false;
		}
	}
	return true;
}

void thome::start_advertising()
{
	const std::string sn = preferences::sn();
	VALIDATE(!sn.empty(), null_str);
	pble_.start_advertising(peripheral_name_, manufacturer_id_, (const uint8_t*)sn.c_str(), sn.size());
}

void thome::click_update_sn(tbutton& widget)
{
	std::string title = find_widget<tlabel>(window_, "sn_label", false, true)->label();
	std::string placeholder = _("ASCII printable characters");
	// gui2::tedit_box_param param(title, null_str, placeholder, null_str, null_str, null_str, _("OK"), MAX_SN_CHARS, gui2::tedit_box_param::show_cancel + 20);
	gui2::tedit_box_param param(title, null_str, placeholder, preferences::sn(), null_str, null_str, _("OK"), MAX_SN_CHARS, gui2::tedit_box_param::show_cancel);
	param.did_text_changed = std::bind(&thome::did_verify_sn, this, _1);
	{
		gui2::tedit_box dlg(param);
		dlg.show(nposm, window_->get_height() / 4);
		if (dlg.get_retval() != twindow::OK) {
			return;
		}
	}

	widget.set_label(param.result);
	preferences::set_sn(param.result);
	if (pble_.is_advertising()) {
		pble_.stop_advertising();
		start_advertising();
	}
}

void thome::app_timer_handler(uint32_t now)
{
	refresh_statusbar_grid(now);

	std::stringstream ss;
	ss.str("");
	if (!pble_.is_advertising()) {
		ss << "Not advertising";

	} else if (pble_.is_connected()) {
		ss << "advertising. Connected" << ": " << pble_.mac_addr().str();

	} else {
		ss << "advertising. Unconnected";
	}
	status_widget_->set_label(ss.str());
}

} // namespace gui2

