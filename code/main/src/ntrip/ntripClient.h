// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Alexandre Petitjean <alpetitjean@gmail.com>
#pragma once

// Project headers
#include <common/tcpConnection.h>
#include <common/types.h>
#include <config/config.h>

namespace sbg::advlogger
{
	class CNtripClient
	{
	public:
		//---------------------------------------------------------------------//
		//- Constructor / Destructor                                          -//
		//---------------------------------------------------------------------//
		explicit CNtripClient(asio::io_context &ioc, const SNtripConfig &ntripConfig);
		~CNtripClient();

		//---------------------------------------------------------------------//
		//- Parameters                                                        -//
		//---------------------------------------------------------------------//
		bool isConnected() const;
		void setGgaFrame(const std::string &gga);

		//---------------------------------------------------------------------//
		//- Operations                                                        -//
		//---------------------------------------------------------------------//
		std::error_code connect();
		void            disconnect();

		void requestSourceTable(SourceTableCallback sourceTableCb, ErrorCallback errorCb = nullptr);
		void startListenMountPoint(RtcmCallback rtcmCb, ErrorCallback errorCb = nullptr);

	private:
		//---------------------------------------------------------------------//
		//- Internal operations                                               -//
		//---------------------------------------------------------------------//
		void scheduleGgaSend();
		void sendGgaFrame();

		//---------------------------------------------------------------------//
		//- Internal data                                                     -//
		//---------------------------------------------------------------------//
		asio::io_context  &m_ioc;
		SNtripConfig       m_ntripConfig;
		CTcpConnection     m_tcpConnection;
		std::string        m_ggaFrame;
		asio::steady_timer m_ggaTimer;
	};

}// namespace sbg::advlogger