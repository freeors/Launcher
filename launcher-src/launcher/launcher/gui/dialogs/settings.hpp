#ifndef GUI_DIALOGS_SETTINGS_HPP_INCLUDED
#define GUI_DIALOGS_SETTINGS_HPP_INCLUDED

#include "gui/dialogs/statusbar.hpp"
#include "gui/dialogs/dialog.hpp"

namespace gui2 {

class tbutton;
class ttoggle_button;

class tsettings: public tdialog, public tstatusbar
{
public:
	explicit tsettings(net::trdpd_manager& rdpd_mgr, tpble2& pble);

private:
	/** Inherited from tdialog. */
	void pre_show() override;

	/** Inherited from tdialog. */
	void post_show() override;

	/** Inherited from tdialog, implemented by REGISTER_DIALOG. */
	virtual const std::string& window_id() const;

	void app_timer_handler(uint32_t now) override;

	void click_getcfg(tbutton& widget);
	void click_setcfg(tbutton& widget);
};

} // namespace gui2

#endif

