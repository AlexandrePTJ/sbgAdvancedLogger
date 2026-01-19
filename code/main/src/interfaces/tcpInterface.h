// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Alexandre Petitjean <alpetitjean@gmail.com>
#pragma once

// Project headers
#include <common/tcpConnection.h>
#include <config/config.h>
#include <interfaces/abstractInterface.h>

namespace sbg::advlogger
{
	class CTcpInterface : public CAbstractInterface
	{
	public:
		//---------------------------------------------------------------------//
		//- Constructor / Destructor                                          -//
		//---------------------------------------------------------------------//
		explicit CTcpInterface(asio::io_context &ioc, const STcpConfig &tcpConfig);
		~CTcpInterface() override;

		// Non-copyable, non-movable
		CTcpInterface(const CTcpInterface &)            = delete;
		CTcpInterface &operator=(const CTcpInterface &) = delete;

		//---------------------------------------------------------------------//
		//- Operations                                                        -//
		//---------------------------------------------------------------------//

		std::error_code start(DataCallback dataCb, ErrorCallback errorCb) override;
		void            stop() override;
		void            asyncWrite(std::span<const uint8_t> data, ErrorCallback errorCb) override;
		bool            isActive() const override;

	private:
		CTcpConnection m_tcpConnection;
	};

}// namespace sbg::advlogger