// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Alexandre Petitjean <alpetitjean@gmail.com>
#pragma once

// STL headers
#include <chrono>
#include <span>

// 3rdparty headers
#include <asio.hpp>

namespace sbg::advlogger
{
	class CTcpConnection;
	class CHttpGetContext
	{
	public:
		enum class ParseState
		{
			Headers,
			Body,
			Complete
		};

		//---------------------------------------------------------------------//
		//- Constructor / Destructor                                          -//
		//---------------------------------------------------------------------//
		explicit CHttpGetContext(asio::io_context &ioc, const std::chrono::seconds &timeout = std::chrono::seconds(10));
		virtual ~CHttpGetContext();

		//---------------------------------------------------------------------//
		//- Parameters                                                        -//
		//---------------------------------------------------------------------//
		bool isComplete() const;

		ParseState getParseState() const;

		std::string        getHeaders() const;
		std::string       &getParseBuffer();
		const std::string &getParseBuffer() const;

		//---------------------------------------------------------------------//
		//- Operations                                                        -//
		//---------------------------------------------------------------------//
		void start(CTcpConnection *tcpConnection = nullptr);
		void append(std::span<const uint8_t> data);

		void complete();
		void cancel();

	private:
		//---------------------------------------------------------------------//
		//- Internal data                                                     -//
		//---------------------------------------------------------------------//
		asio::steady_timer m_timeoutTimer;
		CTcpConnection    *m_tcpConnection = nullptr;

		ParseState  m_parseState = ParseState::Headers;
		std::string m_parseBuffer;
		std::string m_headers;
	};
}// namespace sbg::advlogger
