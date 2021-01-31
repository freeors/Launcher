-- Even if it is empty, must define gui2 for the table type. rose require it.
gui2 = {
	-- <librose>/gui/widgets/widget.hpp
	tvisible_VISIBLE = 0;
	tvisible_HIDDEN = 1;
	tvisible_INVISIBLE = 2;

	-- <librose>/gui/widgets/window.hpp
	twindow_OK = -1;
	twindow_CANCEL = -2;

	-- <librose/gui/dialogs/dialog.hpp>
	tdialog_POST_MSG_MIN_APP = 100;

	-- <librose/gui/widgets/widget.hpp>
	-- tbase_tpl_widget type
	tpl_text_box2 = 1;

	hdpi_scale = 2,
	-- dialogs = {};
}

net = {
	-- <chromium>/net/base/net_errors.h
	OK = 0;
}

lua = {
	LUA_TNIL = 0,
	LUA_TBOOLEAN = 1,
	LUA_TLIGHTUSERDATA = 2,
	-- LUA_TNUMBER = 3,
	LUA_TSTRING = 4,
	LUA_TTABLE = 5,
	LUA_TFUNCTION = 6,
	LUA_TUSERDATA = 7,
	LUA_TTHREAD = 8,

	LUA_VNUMINT = 0x3,
	LUA_VNUMFLT = 0x13,
}

utils = {
	PBIDX_N32TOP0 = 0,
	PBIDX_N32TOP1 = 1,
	PBIDX_N32TOP2 = 2,
	PBIDX_N64TOP0 = 3,
	PBIDX_N64TOP1 = 4,
	PBIDX_N64TOP2 = 5,
	PBIDX_STRTOP0 = 6,
	PBIDX_STRTOP1 = 7,
	PBIDX_STRTOP2 = 8,
	PBIDX_N32ITEM0 = 10,
	PBIDX_N32ITEM1 = 11,
	PBIDX_N32ITEM2 = 12,
	PBIDX_N32ITEM3 = 13,
	PBIDX_N32ITEM4 = 14,
	PBIDX_N32ITEM5 = 15,
	PBIDX_N32ITEM6 = 16,
	PBIDX_N32ITEM7 = 17,
	PBIDX_N32ITEM8 = 18,
	PBIDX_N32ITEM9 = 19,
	PBIDX_N64ITEM0 = 20,
	PBIDX_N64ITEM1 = 21,
	PBIDX_N64ITEM2 = 22,
	PBIDX_STRITEM0 = 23,
	PBIDX_STRITEM1 = 24,
	PBIDX_STRITEM2 = 25,
	PBIDX_STRITEM3 = 26,
	PBIDX_STRITEM4 = 27,

	SIZE_DEFAULT = 26,
	DISABLED_COLOR = 0xff5f5f5f,
}