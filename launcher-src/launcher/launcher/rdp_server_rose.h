// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SERVER_RDP_SERVER_ROSE_H_
#define NET_SERVER_RDP_SERVER_ROSE_H_

#include <freerdp/server/shadow.h>
#include "net/server/rdp_server.h"

#include <gui/dialogs/dialog.hpp>

#include "serialization/string_utils.hpp"
#include "util_c.h"

// webrtc
#include "rtc_base/event.h"
#include "base/threading/thread.h"
#include <wml_exception.hpp>

#include "freerdp/freerdp.h"

enum {rdpdstatus_connectionfinished, rdpdstatus_connectionclosed};

namespace net {

// Bas on HttpServerTest of chromium.
class RdpServerRose: public RdpServer::Delegate
{
	friend class trdpd_manager;
public:
	RdpServerRose(base::Thread& thread, base::WaitableEvent& e);
	~RdpServerRose();

	void SetUp(uint32_t ipaddr);

	void TearDown();

	void OnConnect(RdpConnection& connection) override;

	void OnRdpRequest(RdpConnection::tdid_read_lock& lock, RdpConnection& connection, const uint8_t* buf, int buf_len) override;

	void OnClose(RdpConnection& connection) override;

	const IPEndPoint& server_address() const { return server_address_; }
	const std::string& server_url() const { return server_url_; }

	void clipboard_updated(const std::string& text);
	void hdrop_copied(const std::string& files);
	int hdrop_paste(gui2::tprogress_& progress, const std::string& path, char* err_msg, int max_bytes);
	bool can_hdrop_paste() const;
	void push_explorer_update(uint32_t code, uint32_t data1, uint32_t data2, uint32_t data3);

private:
	void did_connect_bh();
	void rdpd_slice(int timeout);
	void did_slice_quited(int timeout);
	void handle_pause_record_screen(RdpConnection& connection, bool desire_pause);

	void send_startup_msg(uint32_t ticks, int rdpstatus);

protected:
	base::Thread& thread_;
	base::WaitableEvent& delete_pend_tasks_e_;
	std::unique_ptr<RdpServer> server_;
	IPEndPoint server_address_;
	std::string server_url_;

private:
	base::WeakPtrFactory<RdpServerRose> weak_ptr_factory_;
	const int check_slice_timeout_;

	// freerdp section
	const int fake_peer_socket_;
	rdpShadowServer* freerdp_server_;
	SHADOW_GFX_STATUS gfxstatus_;
	void* UpdateSubscriber_;

	bool slice_running_;
	uint32_t startup_verbose_ticks_;
	uint32_t last_verbose_ticks_;
	int client_os_;

	threading::mutex rdpd_thread_explorer_update_mutex_;
	std::vector<LEAGOR_EXPLORER_UPDATE> rdpd_thread_explorer_update_;
};

class trdpd_manager: public tserver_
{
public:
	trdpd_manager();

	const std::string& url() const override
	{ 
		return delegate_.get() != nullptr? delegate_->server_url(): null_str; 
	}

	RdpServer& rdp_server() 
	{ 
		return *delegate_.get()->server_.get();
	}

	bool client_is_mobile() const
	{
		return os_is_mobile(delegate_.get()->client_os_);
	}
	RdpConnection* connectionfinished_connection();

	void clipboard_updated(const std::string& text);
	void hdrop_copied(const std::string& files);
	bool hdrop_paste(gui2::tprogress_& progress, const std::string& path, int* err_code, char* err_msg, int max_bytes);
	bool can_hdrop_paste() const;
	void push_explorer_update(uint32_t code, uint32_t data1, uint32_t data2, uint32_t data3);
	bool support_drag_copy();

private:
	void start(uint32_t ipaddr) override;
	void stop() override;

	void did_set_event();
	void start_internal(uint32_t ipaddr);
	void stop_internal();

private:
	std::unique_ptr<base::Thread> thread_;
	std::unique_ptr<RdpServerRose> delegate_;
	base::WaitableEvent e_;
};

}  // namespace net

#endif // NET_SERVER_RDP_SERVER_ROSE_H_
