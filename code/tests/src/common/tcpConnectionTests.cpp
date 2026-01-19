// Catch2 headers
#include <catch2/catch_test_macros.hpp>

// Project headers
#include <common/tcpConnection.h>

// Tests headers
#include "testTcpEchoServer.h"

using namespace sbg::advlogger;

TEST_CASE("TcpConnection - Connect to local test server", "[tcp]")
{
	asio::io_context ioc;

	test::CTcpEchoServer tcpServer(ioc);

	CTcpConnection tcpConnection(ioc, tcpServer.getClientConfig());
	tcpConnection.asyncConnect([&](const auto & /*ec*/) {});

	ioc.run();

	REQUIRE(tcpConnection.isConnected());
}

TEST_CASE("TcpConnection - Write and Read to echo server", "[tcp]")
{
	asio::io_context ioc;

	test::CTcpEchoServer tcpServer(ioc);

	CTcpConnection tcpConnection(ioc, tcpServer.getClientConfig());

	tcpConnection.asyncConnect([&](const auto & /*ec*/)
	{
		auto res = tcpConnection.write("Hello, World !");
		REQUIRE_FALSE(res);

		tcpConnection.startRead([&](const auto &data)
		{
			REQUIRE(std::string(data.begin(), data.end()) == "Hello, World !");
			tcpConnection.stopRead();
		},
		[](const auto &/*errorCode*/)
		{
			REQUIRE(false);
		}
		);
	});

	// Run until nothing more to do
	ioc.run();
}
