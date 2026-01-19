// Catch2 headers
#include <catch2/catch_test_macros.hpp>

// Project headers
#include <ntrip/ntripClient.h>

using namespace sbg::advlogger;

static const auto gCentipedeConf = SNtripConfig{
    .host       = "crtk.net",
    .port       = 2101,
    .mountpoint = "NEAR",
    .username   = "centipede",
    .password   = "centipede"};

TEST_CASE("NTRIP Client - connect", "[ntrip]")
{
	asio::io_context ioc;

	CNtripClient ntripClient(ioc, gCentipedeConf);
	auto         err = ntripClient.connect();

	REQUIRE(err.value() == 0);
	REQUIRE(ntripClient.isConnected());
}

TEST_CASE("NTRIP Client - Get source table", "[ntrip]")
{
	asio::io_context ioc;

	CNtripClient ntripClient(ioc, gCentipedeConf);
	auto         err = ntripClient.connect();

	REQUIRE(err.value() == 0);

	std::string sourceTable;

	ntripClient.requestSourceTable([&](const auto &st)
	                               { sourceTable = st; },
	                               [](const auto & /*ec*/)
	                               { REQUIRE(false); });

	ioc.restart();
	ioc.run();

	REQUIRE(!sourceTable.empty());
}