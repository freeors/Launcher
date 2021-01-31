/* $Id: campaign_difficulty.cpp 49602 2011-05-22 17:56:13Z mordante $ */
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

#define GETTEXT_DOMAIN "launcher-lib"

#include "gui/dialogs/explorer.hpp"

#include "formula_string_utils.hpp"
#include "gettext.hpp"
#include "filesystem.hpp"
#include "rose_config.hpp"

#include "gui/dialogs/helper.hpp"
#include "gui/widgets/settings.hpp"
#include "gui/widgets/window.hpp"
#include "gui/widgets/label.hpp"
#include "gui/widgets/button.hpp"
#include "gui/widgets/toggle_button.hpp"
#include "gui/widgets/toggle_panel.hpp"
#include "gui/widgets/listbox.hpp"
#include "gui/widgets/report.hpp"
#include "gui/widgets/text_box.hpp"
#include "gui/widgets/stack.hpp"
#include "gui/dialogs/combo_box.hpp"
#include "gui/dialogs/message.hpp"
#include "gui/dialogs/menu.hpp"
#include "gui/dialogs/edit_box.hpp"
#include "gui/dialogs/explorer.hpp"

#include "game_config.hpp"
#include "base_instance.hpp"
#include <freerdp/channels/cliprdr_common2.hpp>

namespace gui2 {

threading::mutex texplorer_slot::construct_mutex;

texplorer_slot::texplorer_slot(net::trdpd_manager& rdpd_mgr, tpble2& pble)
	: tstatusbar(rdpd_mgr, pble)
{
	game_config::explorer_singleton = this;

	rdpd_mgr_.push_explorer_update(LG_EXPLORER_CODE_SHOWN, game_config::statusbar_height, game_config::navigation_height, 0);
}

void texplorer_slot::rexplorer_pre_show(twindow& window)
{
	drag_copy_.support = rdpd_mgr_.support_drag_copy();
	if (drag_copy_.support) {
		find_widget<tbutton>(explorer_grid_, "new_folder", false, true)->set_visible(twidget::INVISIBLE);
	}

	tgrid* grid_ptr = main_stack_->layer(trexplorer::EXPLORER_LAYER);
	main_stack_->set_did_pip_top_grid_rect(*grid_ptr, std::bind(&texplorer_slot::did_pip_top_grid_rect, this, _1, _2, _3, _4));

	tstatusbar::pre_show(window, find_widget<tpanel>(&window, "statusbar_panel", false).grid());
}

void texplorer_slot::rexplorer_post_show()
{	
	rdpd_mgr_.push_explorer_update(LG_EXPLORER_CODE_HIDDEN, 0, 0, 0);

	rexplorer_->send_helper().clear_msg();

	threading::lock lock(construct_mutex);
	game_config::explorer_singleton = nullptr;
}

void texplorer_slot::send_LG_EXPLORER_CODE_SHOWN()
{
	rdpd_mgr_.push_explorer_update(LG_EXPLORER_CODE_SHOWN, game_config::statusbar_height, game_config::navigation_height, 0);
	if (drag_copy_.support) {
		main_stack_->pip_relayout();
	}
}

void texplorer_slot::send_LG_EXPLORER_CODE_HIDDEN()
{
	rdpd_mgr_.push_explorer_update(LG_EXPLORER_CODE_HIDDEN, 0, 0, 0);
	if (drag_copy_.support) {
		main_stack_->pip_relayout();
	}
}

void texplorer_slot::rdp_copy(tlistbox& list)
{
	std::vector<std::string> files;
	if (list.cursel() != nullptr) {
		files.push_back(rexplorer_->selected_full_name(nullptr));
	} else {
		files = rexplorer_->selected_full_multinames();
	}
	std::string files_str = utils::join_with_null(files);
	rdpd_mgr_.hdrop_copied(files_str);
}

void texplorer_slot::rdp_paste(tlistbox& list)
{
	int err_code = cliprdr_errcode_ok;
	char err_msg[512];

	SDL_Rect track_rect{nposm, nposm, nposm, nposm};
	if (drag_copy_.support) {
		// temporary solution for rdpd paste and support DRAG_COPY(1/2).
		const int half_width = window_->get_width() / 2;
		track_rect.x = half_width / 20;
		track_rect.w = half_width * 9 / 10;
	}

	// const std::string title = cliprdr_msgid_2_str(cliprdr_msgid_pastingwarn, game_config::get_app_msgstr(null_str));
	utils::string_map symbols;
	symbols["src"] = _("Local");
	symbols["dst"] = _("Remote");
	const std::string title = vgettext2("$src ---> $dst", symbols);
	tprogress_default_slot slot(std::bind(&net::trdpd_manager::hdrop_paste, &rdpd_mgr_, _1, rexplorer_->current_dir(), &err_code, err_msg, sizeof(err_msg)), "misc/remove.png", gui2::tgrid::HORIZONTAL_ALIGN_LEFT);
	gui2::run_with_progress(slot, null_str, null_str, 0, track_rect);
	const std::string msg = cliprdr_code_2_str(err_msg, err_code);
	if (!msg.empty()) {
		gui2::show_message(null_str, msg);
	}
	rexplorer_->update_file_lists();
}

void texplorer_slot::rexplorer_click_edit(tbutton& widget)
{
	bool has_handshaked_connection = false;
	if (rdpd_mgr_.started()) {
		net::RdpServer& server = rdpd_mgr_.rdp_server();
		has_handshaked_connection = server.FindFirstHandshakedConnection() != nullptr;
	}
	if (!has_handshaked_connection) {
		gui2::show_message(null_str, cliprdr_msgid_2_str(cliprdr_msgid_noclient, null_str));
		return;
	}

	enum {copy, cut, paste};

	std::vector<gui2::tmenu::titem> items;
	
	if (file_list_->cursel() != nullptr || !file_list_->multiselected_rows().empty()) {
		items.push_back(gui2::tmenu::titem(cliprdr_msgid_2_str(cliprdr_msgid_copy, null_str), copy));
		// items.push_back(gui2::tmenu::titem(_("Cut"), cut));
	}
	if (rdpd_mgr_.can_hdrop_paste()) {
		// paste
		items.push_back(gui2::tmenu::titem(cliprdr_msgid_2_str(cliprdr_msgid_paste, null_str), paste));
	}

	if (items.empty()) {
		gui2::show_message(null_str, cliprdr_msgid_2_str(cliprdr_msgid_nooperator, null_str));
		return;
	}

	int selected;
	{
		gui2::tmenu dlg(items, nposm);
		dlg.show(widget.get_x(), widget.get_y() + widget.get_height() + 16 * twidget::hdpi_scale);
		int retval = dlg.get_retval();
		if (dlg.get_retval() != gui2::twindow::OK) {
			return;
		}
		// absolute_draw();
		selected = dlg.selected_val();
	}

	if (selected == copy) {
		rdp_copy(*file_list_);

	} else if (selected == paste) {
		rdp_paste(*file_list_);
		
	} else if (selected == cut) {
		// cut_selection();
	}
}

void texplorer_slot::rexplorer_timer_handler(uint32_t now)
{
	refresh_statusbar_grid(now);
}

void texplorer_slot::did_rdpd_client_connectionfinished()
{
	SDL_Log("texplorer_slot::did_rdpd_client_connectionfinished---");
	VALIDATE(!drag_copy_.draging, null_str);
	if (drag_copy_.support) {
		main_stack_->pip_relayout();
	}
	// full --> left
}

void texplorer_slot::did_rdpd_client_closed()
{
	SDL_Log("texplorer_slot::did_rdpd_client_closed---");
	if (drag_copy_.draging) {
		SDL_Log("texplorer_slot::did_rdpd_client_closed, drag_copy_.draging is true, rest");
		if (!drag_copy_.src_is_server) {
			// require left up. but no simulate way.
/*
			SDL_Event e;
			memset(&e, 0, sizeof(e));
			e.type = SDL_WINDOWEVENT;
			e.window.event = SDL_WINDOWEVENT_LEAVE;
			gui2::send_sdl_event(e); 
*/
		}
		drag_copy_.reset();
	}

	// left --> full
	// Theoretically, here should called main_stack_->pip_relayout(). 
	// However, when closed, other widget will trigger re-layout window, as id="client_ip" widget in statusbar . 
	// To reduce the number of layouts, it is not called here.
	// if (drag_copy_.support) {
		// main_stack_->pip_relayout();
	// }
	drag_copy_.support = false;

	find_widget<tbutton>(explorer_grid_, "new_folder", false, true)->set_visible(twidget::VISIBLE);
}

void texplorer_slot::rexplorer_OnMessage(rtc::Message* msg)
{
	const uint32_t now = SDL_GetTicks();

	switch (msg->message_id) {
	case MSG_STARTUP_MSG:
		{
			tmsg_startup_msg* pdata = static_cast<tmsg_startup_msg*>(msg->pdata);
			VALIDATE(pdata->ticks != 0, null_str);
			if (pdata->rdpstatus == rdpdstatus_connectionfinished) {
				did_rdpd_client_connectionfinished();

			} else if (pdata->rdpstatus == rdpdstatus_connectionclosed) {
				did_rdpd_client_closed();
			}
		}
		break;

	case MSG_CAPABILITIES:
		{
			tmsg_capabilities* pdata = static_cast<tmsg_capabilities*>(msg->pdata);
			const LEAGOR_CAPABILITIES& capabilities = pdata->capabilities;

			drag_copy_.support = (capabilities.flags & LG_CAP_DROP_COPY)? true: false;
			if (drag_copy_.support && instance->foreground()) {
				send_LG_EXPLORER_CODE_SHOWN();
				if (drag_copy_.support) {
					find_widget<tbutton>(explorer_grid_, "new_folder", false, true)->set_visible(twidget::INVISIBLE);
				}
			}
		}
		break;

	case MSG_EXPLORER:
		{
			tmsg_explorer* pdata = static_cast<tmsg_explorer*>(msg->pdata);
			const LEAGOR_EXPLORER_UPDATE& update = pdata->update;

			bool update_explorer_grid = false;
			if (update.code == LG_EXPLORER_CODE_START_DRAG) {
				SDL_Log("rexplorer_OnMessage, LG_EXPLORER_CODE_START_DRAG, drag_copy_.draging: %s", drag_copy_.draging? "true": "false");
				int x = update.data1;
				int y = update.data2;
				drag_copy_.draging = true;
				drag_copy_.files = update.data3 & LG_START_DRAG_FILES_MASK;
				drag_copy_.has_folder = update.data3 & LG_START_DRAG_HAS_FOLDER_FLAG? true: false;
				drag_copy_.src_is_server = false;

			} else if (update.code == LG_EXPLORER_CODE_END_DRAG) {
				SDL_Log("rexplorer_OnMessage, LG_EXPLORER_CODE_END_DRAG");
				int x = update.data1;
				int y = update.data2;
				bool up_result = update.data3 & LG_END_DRAG_UP_RESULT_FLAG? true: false;
				if (drag_copy_.src_is_server) {
					if (!file_list_->multiselect()) {
						// selected is a directory, which must be set to unselected for return to normal state.
						VALIDATE(file_list_->cursel() != nullptr, null_str);
						trexplorer::tfile2 file = rexplorer_->row_2_file2(file_list_->cursel()->at());
						if (file.dir) {
							file_list_->select_row(nposm);
						}
					}
				} else {
					if (up_result) {
						SDL_Rect widget_rect = file_list_->get_rect();
						if (point_in_rect(x, y, widget_rect)) {
							rdp_paste(*file_list_);
						}
					}
				}
				drag_copy_.reset();
			}
		}
		break;
	}
}

SDL_Rect texplorer_slot::did_pip_top_grid_rect(tstack& widget, tgrid& grid, int width, int height)
{
	bool half = rdpd_mgr_.connectionfinished_connection() != nullptr && drag_copy_.support && instance->foreground();
	int width2 = half? width / 2: width;
	if (height == nposm) {
		return SDL_Rect{0, 0, width2, height};
	}

	return SDL_Rect{0, 0, width2, height};
}

void texplorer_slot::rexplorer_handler_longpress_item(const tpoint& coordinate, ttoggle_panel& widget)
{
	if (rdpd_mgr_.connectionfinished_connection() == nullptr) {
		trexplorer::tfile2 file = rexplorer_->row_2_file2(widget.at());
		// if (!file.dir || file_list_->multiselect()) {
		if (!file.dir) {
			// file_list_->select_row(widget.at());
		}
		return;
	}

	drag_copy_.src_is_server = true;
	drag_copy_.draging = true;
	// if src_is_server = false, scrollable of listbox in kDesktop is disable by window->draging()=true.
	// src_is_server requrie extra disable scrollable.
	drag_copy_.scroll_lock.reset(new tscroll_container::trefuse_scroll_lock);
	{
		trexplorer::tdisable_click_open_dir_lock lock(*rexplorer_);
		file_list_->select_row(widget.at());
	}
	rdp_copy(*file_list_);

	SDL_Point selecteds = rexplorer_->analysis_selected();
	drag_copy_.files = selecteds.x + selecteds.y;
	drag_copy_.has_folder = selecteds.x > 0;
	uint32_t data3 = LG_START_DRAG_FORM_DATA3(drag_copy_.files, drag_copy_.has_folder? 1: 0);
	rdpd_mgr_.push_explorer_update(LG_EXPLORER_CODE_START_DRAG, coordinate.x, coordinate.y, data3);
}

void texplorer_slot::rexplorer_handler_mouse_motion(const int x, const int y)
{
	if (drag_copy_.draging && !drag_copy_.src_is_server) {
		ttoggle_panel* row = file_list_->visible_area_point_2_row(x, y);
		// SDL_Log("texplorer_slot::rexplorer_handler_mouse_motion, (%i, %i) ==> %i", x, y, row != nullptr? row->at(): -1);

		SDL_Rect widget_rect = file_list_->get_rect();
		if (point_in_rect(x, y, widget_rect)) {
			if (!drag_copy_.curr_can_paste) {
				drag_copy_.curr_can_paste = true;
				rdpd_mgr_.push_explorer_update(LG_EXPLORER_CODE_CAN_PASTE, x, y, 1);
			}
		} else if (drag_copy_.curr_can_paste) {
			drag_copy_.curr_can_paste = false;
			rdpd_mgr_.push_explorer_update(LG_EXPLORER_CODE_CAN_PASTE, x, y, 0);
		}
	}
}

}
