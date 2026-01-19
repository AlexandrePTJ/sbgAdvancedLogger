#include "testTcpEchoServer.h"

using namespace sbg::advlogger;
using namespace sbg::advlogger::test;

CTcpEchoServer::CTcpEchoServer(asio::io_context &ioc, uint16_t port):
m_ioc(ioc),
m_acceptor(ioc, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
m_socket(ioc)
{
	// clang-format off
	m_thread = std::thread([this]()
	{
		try
		{
			m_acceptor.accept(m_socket);

			std::array<char, 1024> data;
			while (true)
			{
				asio::error_code ec;
				size_t len = m_socket.read_some(asio::buffer(data), ec);

				if (ec) {break;}
				asio::write(m_socket, asio::buffer(data, len));
			}
		} catch (...) { }
	});
	// clang-format on
}

CTcpEchoServer::~CTcpEchoServer()
{
	m_ioc.stop();
	if (m_socket.is_open())
	{
		asio::error_code ec;
		m_socket.close(ec);
	}
	if (m_thread.joinable())
	{
		m_thread.join();
	}
}

uint16_t CTcpEchoServer::getListeningPort() const
{
	return m_acceptor.local_endpoint().port();
}

STcpConfig CTcpEchoServer::getClientConfig() const
{
	return {.host = "localhost", .port = getListeningPort()};
}