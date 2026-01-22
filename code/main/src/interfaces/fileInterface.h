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
	class CFileInterface : public CAbstractInterface
	{
	public:
		//---------------------------------------------------------------------//
		//- Constructor / Destructor                                          -//
		//---------------------------------------------------------------------//
		explicit CFileInterface(asio::io_context &ioc, const SFileConfig &config);
		~CFileInterface() override;

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
		void scheduleRead();
		void doRead();

		//---------------------------------------------------------------------//
		//- Internal data                                                     -//
		//---------------------------------------------------------------------//
		SFileConfig m_fileConfig;

		DataCallback  m_dataCb;
		ErrorCallback m_errorCb;

		asio::steady_timer                 m_readTimer;
		std::atomic<bool>                  m_isRunning;
		std::unique_ptr<asio::stream_file> m_file;
	};

}// namespace sbg::advlogger