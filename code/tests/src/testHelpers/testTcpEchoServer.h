#pragma once

// STL headers
#include <thread>

// 3rdparty headers
#include <asio.hpp>

// Project headers
#include <config/config.h>

namespace sbg::advlogger::test
{
	class CTcpEchoServer
	{
	public:
		CTcpEchoServer(asio::io_context& ioc, uint16_t port = 8080);
		~CTcpEchoServer();

		uint16_t getListeningPort() const;
		STcpConfig getClientConfig() const;

	private:
		asio::io_context&        m_ioc;
		asio::ip::tcp::acceptor m_acceptor;
		asio::ip::tcp::socket   m_socket;
		std::thread             m_thread;
	};
}// namespace sbg::advlogger::test
