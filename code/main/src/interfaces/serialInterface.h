// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Alexandre Petitjean <alpetitjean@gmail.com>
#pragma once

// STL headers
#include <array>

// 3rdparty headers
#include <asio.hpp>

// Project headers
#include <config/config.h>
#include <interfaces/abstractInterface.h>

namespace sbg::advlogger
{
	class CSerialInterface : public CAbstractInterface
	{
	public:
		//---------------------------------------------------------------------//
		//- Constructor / Destructor                                          -//
		//---------------------------------------------------------------------//
		explicit CSerialInterface(asio::io_context &ioc, const SSerialConfig &config);
		~CSerialInterface() override;

		//---------------------------------------------------------------------//
		//- Operations                                                        -//
		//---------------------------------------------------------------------//
		std::error_code start(DataCallback dataCb, ErrorCallback errorCb) override;
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
		asio::serial_port m_serialPort;

		SSerialConfig m_serialConfig;

		std::array<uint8_t, 8192> m_readBuffer;
		DataCallback              m_dataCb;
		ErrorCallback             m_errorCb;
	};

}// namespace sbg::advlogger