/* $Id: campaign_difficulty.hpp 49603 2011-05-22 17:56:17Z mordante $ */
/*
   Copyright (C) 2010 - 2011 by Ignacio Riquelme Morelle <shadowm2006@gmail.com>


   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY.

   See the COPYING file for more details.
*/

#ifndef GUI_DIALOGS_EXPLORER_HPP_INCLUDED
#define GUI_DIALOGS_EXPLORER_HPP_INCLUDED

#include "gui/dialogs/rexplorer.hpp"
#include "gui/dialogs/statusbar.hpp"
#include "gui/widgets/listbox.hpp"

namespace gui2 {
class tstack;
class tgrid;

class texplorer_slot: public trexplorer::tslot, public tstatusbar
{
public:
	static threading::mutex construct_mutex;
	explicit texplorer_slot(net::trdpd_manager& rdpd_mgr, tpble2& pble);


	void send_LG_EXPLORER_CODE_SHOWN();
	void send_LG_EXPLORER_CODE_HIDDEN();

	struct tmsg_startup_msg: public rtc::MessageData {
		explicit tmsg_startup_msg(uint32_t ticks, int rdpstatus)
			: ticks(ticks)
			, rdpstatus(rdpstatus)
		{}

		const uint32_t ticks;
		const int rdpstatus;
	};

	struct tmsg_capabilities: public rtc::MessageData {
		tmsg_capabilities(const LEAGOR_CAPABILITIES& capabilities)
			: capabilities(capabilities)
		{}
		const LEAGOR_CAPABILITIES capabilities;
	};

	struct tmsg_explorer: public rtc::MessageData {
		tmsg_explorer(const LEAGOR_EXPLORER_UPDATE update)
			: update(update)
		{}
		LEAGOR_EXPLORER_UPDATE update;
	};

	enum {MSG_STARTUP_MSG = trexplorer::POST_MSG_MIN_SLOT, MSG_CAPABILITIES, MSG_EXPLORER, MSG_VISIBLE_PERCENT};

private:
	void rexplorer_pre_show(twindow& window) override;
	void rexplorer_post_show() override;
	void rexplorer_click_edit(tbutton& widget) override;
	void rexplorer_timer_handler(uint32_t now) override;
	void rexplorer_OnMessage(rtc::Message* msg) override;

	void rexplorer_handler_mouse_motion(const int x, const int y) override;
	void rexplorer_handler_longpress_item(const tpoint& coordinate, ttoggle_panel& widget) override;

	SDL_Rect did_pip_top_grid_rect(tstack& widget, tgrid& grid, int width, int height);
	void did_rdpd_client_connectionfinished();
	void did_rdpd_client_closed();

	void rdp_copy(tlistbox& list);
	void rdp_paste(tlistbox& list);

private:
	struct tdrag_copy
	{
		tdrag_copy()
			: support(false)
		{
			reset();
		}

		void reset()
		{
			draging = false;
			src_is_server = false;
			files = 0;
			has_folder = false;
			curr_can_paste = false;
			scroll_lock.reset();
		}

		bool support;
		bool draging;
		bool src_is_server;
		int files;
		bool has_folder;
		bool curr_can_paste;
		std::unique_ptr<tscroll_container::trefuse_scroll_lock> scroll_lock;
	};
	tdrag_copy drag_copy_;
};


}

#endif /* ! GUI_DIALOGS_EXPLORER_HPP_INCLUDED */
