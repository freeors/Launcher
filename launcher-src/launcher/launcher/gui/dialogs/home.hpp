#ifndef GUI_DIALOGS_HOME_HPP_INCLUDED
#define GUI_DIALOGS_HOME_HPP_INCLUDED

#include "gui/dialogs/statusbar.hpp"
#include "gui/dialogs/dialog.hpp"

namespace gui2 {

class tbutton;
class ttoggle_button;
class ttext_box;

class thome: public tdialog, public tstatusbar
{
public:
	enum tresult {EXPLORER = 1, SETTINGS};

	explicit thome(net::trdpd_manager& rdpd_mgr, tpble2& pble);

private:
	/** Inherited from tdialog. */
	void pre_show() override;

	/** Inherited from tdialog. */
	void post_show() override;

	/** Inherited from tdialog, implemented by REGISTER_DIALOG. */
	virtual const std::string& window_id() const;

	void app_timer_handler(uint32_t now) override;

	void click_aismart();
	void click_explorer();
	void click_settings(tbutton& widget);
	void did_bleperipheral_changed(ttoggle_button& widget);
	bool did_verify_sn(const std::string& label);
	void click_update_sn(tbutton& widget);

	void start_advertising();

private:
	const std::string peripheral_name_;
	const int manufacturer_id_;
	uint32_t start_ticks_;
	tlabel* status_widget_;
};

} // namespace gui2

#endif

