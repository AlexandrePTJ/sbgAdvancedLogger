// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Alexandre Petitjean <alpetitjean@gmail.com>
#pragma once

// STL headers
#include <array>
#include <functional>
#include <span>

// 3rdparty headers
#include <asio.hpp>

// Project headers
#include <config/config.h>
#include <interfaces/abstractInterface.h>

namespace sbg::advlogger
{
	class CUdpInterface : public CAbstractInterface
	{
	public:
		//---------------------------------------------------------------------//
		//- Constructor / Destructor                                          -//
		//---------------------------------------------------------------------//
		explicit CUdpInterface(asio::io_context &ioc, const SUdpConfig &config);
		~CUdpInterface() override;

		// Non-copyable, non-movable
		CUdpInterface(const CUdpInterface &)            = delete;
		CUdpInterface &operator=(const CUdpInterface &) = delete;

		//---------------------------------------------------------------------//
		//- Operations                                                        -//
		//---------------------------------------------------------------------//
		std::error_code start(DataCallback data_cb, ErrorCallback error_cb) override;
		void            stop() override;
		void            asyncWrite(std::span<const uint8_t> data, ErrorCallback errorCb) override;

	private:
		//---------------------------------------------------------------------//
		//- Internal operations                                               -//
		//---------------------------------------------------------------------//
		void doRead();

		//---------------------------------------------------------------------//
		//- Internal data                                                     -//
		//---------------------------------------------------------------------//
		SUdpConfig                m_udpConfig;
		asio::ip::udp::socket     m_socket;
		asio::ip::udp::endpoint   m_remoteInEndpoint; // Sender's endpoint (updated on receive)
		asio::ip::udp::endpoint   m_remoteOutEndpoint;// Destination's endpoint (write)
		std::array<uint8_t, 8192> m_readBuffer;
		DataCallback              m_dataCb;
		ErrorCallback             m_errorCb;
	};

}// namespace sbg::advlogger