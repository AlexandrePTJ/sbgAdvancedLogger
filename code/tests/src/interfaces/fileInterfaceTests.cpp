// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Alexandre Petitjean <alpetitjean@gmail.com>

// Catch2 headers
#include <catch2/catch_test_macros.hpp>

// STL headers
#include <chrono>
#include <filesystem>
#include <thread>
#include <vector>

// 3rdparty headers
#include <asio.hpp>

// Project headers
#include <interfaces/fileInterface.h>

using namespace sbg::advlogger;
using namespace std::chrono_literals;

namespace
{
	const std::filesystem::path kTestDataDir = TESTS_DATA_DIR_PATH;

	struct TestContext
	{
		asio::io_context                  ioc;
		std::vector<std::vector<uint8_t>> receivedData;
		std::vector<std::error_code>      errors;
		std::atomic<int>                  callbackCount{0};
		std::thread                       ioThread;

		void start()
		{
			ioThread = std::thread([this]()
			                       { ioc.run(); });
		}

		void stop()
		{
			ioc.stop();
			if (ioThread.joinable())
			{
				ioThread.join();
			}
		}

		~TestContext()
		{
			stop();
		}
	};
}// namespace

TEST_CASE("CFileInterface - Binary file reading", "[fileInterface]")
{
	SECTION("Read binary file in chunks")
	{
		TestContext ctx;
		SFileConfig config{
		    .path            = kTestDataDir / "binary_10bytes.bin",
		    .isText          = false,
		    .chunkSize       = 10,
		    .sendingInterval = 50ms,
		    .loop            = false};

		CFileInterface fileInterface(ctx.ioc, config);

		auto dataCb = [&](std::span<const uint8_t> data)
		{
			ctx.receivedData.emplace_back(data.begin(), data.end());
			++ctx.callbackCount;
		};

		auto errorCb = [&](std::error_code ec)
		{
			ctx.errors.push_back(ec);
		};

		auto ec = fileInterface.start(dataCb, errorCb);
		REQUIRE(!ec);

		ctx.start();
		std::this_thread::sleep_for(150ms);
		fileInterface.stop();
		ctx.stop();

		REQUIRE(ctx.callbackCount >= 1);
		REQUIRE(ctx.receivedData.size() >= 1);
		REQUIRE(ctx.receivedData[0].size() == 10);

		// Verify content
		std::string content(ctx.receivedData[0].begin(), ctx.receivedData[0].end());
		REQUIRE(content == "0123456789");
	}

	SECTION("Read binary file in multiple chunks")
	{
		TestContext ctx;
		SFileConfig config{
		    .path            = kTestDataDir / "binary_50bytes.bin",
		    .isText          = false,
		    .chunkSize       = 20,
		    .sendingInterval = 50ms,
		    .loop            = false};

		CFileInterface fileInterface(ctx.ioc, config);

		auto dataCb = [&](std::span<const uint8_t> data)
		{
			ctx.receivedData.emplace_back(data.begin(), data.end());
			++ctx.callbackCount;
		};

		auto errorCb = [&](std::error_code ec)
		{
			ctx.errors.push_back(ec);
		};

		auto ec = fileInterface.start(dataCb, errorCb);
		REQUIRE(!ec);

		ctx.start();
		std::this_thread::sleep_for(250ms);
		fileInterface.stop();
		ctx.stop();

		// Should read 3 chunks: 20 + 20 + 10 bytes
		REQUIRE(ctx.callbackCount >= 2);
	}

	SECTION("Binary file with loop enabled")
	{
		TestContext ctx;
		SFileConfig config{
		    .path            = kTestDataDir / "binary_10bytes.bin",
		    .isText          = false,
		    .chunkSize       = 10,
		    .sendingInterval = 50ms,
		    .loop            = true};

		CFileInterface fileInterface(ctx.ioc, config);

		auto dataCb = [&](std::span<const uint8_t> data)
		{
			ctx.receivedData.emplace_back(data.begin(), data.end());
			++ctx.callbackCount;
		};

		auto errorCb = [&](std::error_code ec)
		{
			ctx.errors.push_back(ec);
		};

		auto ec = fileInterface.start(dataCb, errorCb);
		REQUIRE(!ec);

		ctx.start();
		std::this_thread::sleep_for(350ms);
		fileInterface.stop();
		ctx.stop();

		// Should read a file multiple times with looping enabled
		REQUIRE(ctx.callbackCount >= 3);

		// All chunks should be identical
		for (const auto &chunk : ctx.receivedData)
		{
			std::string content(chunk.begin(), chunk.end());
			REQUIRE(content == "0123456789");
		}
	}
}

TEST_CASE("CFileInterface - Text file line-by-line reading", "[fileInterface]")
{
	SECTION("Read text file line by line")
	{
		TestContext ctx;
		SFileConfig config{
		    .path               = kTestDataDir / "text_lines_with_newline.txt",
		    .isText             = true,
		    .textChunkToNewLine = true,
		    .chunkSize          = 0,
		    .sendingInterval    = 50ms,
		    .loop               = false};

		CFileInterface fileInterface(ctx.ioc, config);

		auto dataCb = [&](std::span<const uint8_t> data)
		{
			ctx.receivedData.emplace_back(data.begin(), data.end());
			++ctx.callbackCount;
		};

		auto errorCb = [&](std::error_code ec)
		{
			ctx.errors.push_back(ec);
		};

		auto ec = fileInterface.start(dataCb, errorCb);
		REQUIRE(!ec);

		ctx.start();
		std::this_thread::sleep_for(400ms);
		fileInterface.stop();
		ctx.stop();

		// Should read 5 lines
		REQUIRE(ctx.callbackCount >= 4);

		if (ctx.receivedData.size() >= 1)
		{
			std::string firstLine(ctx.receivedData[0].begin(), ctx.receivedData[0].end());
			REQUIRE(firstLine == "First line\n");
		}
	}

	SECTION("Text file without trailing newline")
	{
		TestContext ctx;
		SFileConfig config{
		    .path               = kTestDataDir / "text_lines.txt",
		    .isText             = true,
		    .textChunkToNewLine = true,
		    .chunkSize          = 0,
		    .sendingInterval    = 50ms,
		    .loop               = false};

		CFileInterface fileInterface(ctx.ioc, config);

		auto dataCb = [&](std::span<const uint8_t> data)
		{
			ctx.receivedData.emplace_back(data.begin(), data.end());
			++ctx.callbackCount;
		};

		auto errorCb = [&](std::error_code ec)
		{
			ctx.errors.push_back(ec);
		};

		auto ec = fileInterface.start(dataCb, errorCb);
		REQUIRE(!ec);

		ctx.start();
		std::this_thread::sleep_for(400ms);
		fileInterface.stop();
		ctx.stop();

		// Should read all lines including last one without newline
		REQUIRE(ctx.callbackCount >= 4);
	}
}

TEST_CASE("CFileInterface - Text file chunk reading", "[fileInterface]")
{
	SECTION("Read text file in fixed chunks")
	{
		TestContext ctx;
		SFileConfig config{
		    .path               = kTestDataDir / "text_lines.txt",
		    .isText             = true,
		    .textChunkToNewLine = false,
		    .chunkSize          = 20,
		    .sendingInterval    = 50ms,
		    .loop               = false};

		CFileInterface fileInterface(ctx.ioc, config);

		auto dataCb = [&](std::span<const uint8_t> data)
		{
			ctx.receivedData.emplace_back(data.begin(), data.end());
			++ctx.callbackCount;
		};

		auto errorCb = [&](std::error_code ec)
		{
			ctx.errors.push_back(ec);
		};

		auto ec = fileInterface.start(dataCb, errorCb);
		REQUIRE(!ec);

		ctx.start();
		std::this_thread::sleep_for(250ms);
		fileInterface.stop();
		ctx.stop();

		REQUIRE(ctx.callbackCount >= 1);

		// First chunk should be 20 bytes or less
		if (ctx.receivedData.size() >= 1)
		{
			REQUIRE(ctx.receivedData[0].size() <= 20);
		}
	}
}

TEST_CASE("CFileInterface - Configuration validation", "[fileInterface]")
{
	asio::io_context ioc;

	SECTION("Invalid file path returns error")
	{
		SFileConfig config{
		    .path            = kTestDataDir / "nonexistent.bin",
		    .isText          = false,
		    .chunkSize       = 10,
		    .sendingInterval = 50ms,
		    .loop            = false};

		CFileInterface fileInterface(ioc, config);

		auto dataCb  = [](std::span<const uint8_t>) {};
		auto errorCb = [](std::error_code) {};

		auto ec = fileInterface.start(dataCb, errorCb);
		REQUIRE(ec);
	}

	SECTION("Directory path returns error")
	{
		SFileConfig config{
		    .path            = kTestDataDir,
		    .isText          = false,
		    .chunkSize       = 10,
		    .sendingInterval = 50ms,
		    .loop            = false};

		CFileInterface fileInterface(ioc, config);

		auto dataCb  = [](std::span<const uint8_t>) {};
		auto errorCb = [](std::error_code) {};

		auto ec = fileInterface.start(dataCb, errorCb);
		REQUIRE(ec);
	}
}

TEST_CASE("CFileInterface - Timing and intervals", "[fileInterface]")
{
	SECTION("Respects sendingInterval between reads")
	{
		TestContext ctx;
		SFileConfig config{
		    .path            = kTestDataDir / "binary_10bytes.bin",
		    .isText          = false,
		    .chunkSize       = 10,
		    .sendingInterval = 100ms,
		    .loop            = true};

		CFileInterface fileInterface(ctx.ioc, config);

		std::vector<std::chrono::steady_clock::time_point> timestamps;

		auto dataCb = [&](std::span<const uint8_t> /*data*/)
		{
			timestamps.push_back(std::chrono::steady_clock::now());
			++ctx.callbackCount;
		};

		auto errorCb = [&](std::error_code ec)
		{
			ctx.errors.push_back(ec);
		};

		auto ec = fileInterface.start(dataCb, errorCb);
		REQUIRE(!ec);

		ctx.start();
		std::this_thread::sleep_for(350ms);
		fileInterface.stop();
		ctx.stop();

		// Should have at least 2 reads to measure an interval
		REQUIRE(timestamps.size() >= 2);

		// Check an interval between consecutive reads (allowing 20ms tolerance)
		for (size_t i = 1; i < timestamps.size(); ++i)
		{
			auto interval = std::chrono::duration_cast<std::chrono::milliseconds>(timestamps[i] - timestamps[i - 1]);
			REQUIRE(interval >= 80ms);
			REQUIRE(interval <= 120ms);
		}
	}
}

TEST_CASE("CFileInterface - Start/Stop lifecycle", "[fileInterface]")
{
	SECTION("Stop prevents further reads")
	{
		TestContext ctx;
		SFileConfig config{
		    .path            = kTestDataDir / "binary_10bytes.bin",
		    .isText          = false,
		    .chunkSize       = 10,
		    .sendingInterval = 50ms,
		    .loop            = true};

		CFileInterface fileInterface(ctx.ioc, config);

		auto dataCb = [&](std::span<const uint8_t> /*data*/)
		{
			++ctx.callbackCount;
		};

		auto errorCb = [&](std::error_code ec)
		{
			ctx.errors.push_back(ec);
		};

		auto ec = fileInterface.start(dataCb, errorCb);
		REQUIRE(!ec);

		ctx.start();
		std::this_thread::sleep_for(100ms);

		int countBeforeStop = ctx.callbackCount.load();
		fileInterface.stop();

		std::this_thread::sleep_for(150ms);
		ctx.stop();

		// Count should not increase significantly after stop
		REQUIRE(ctx.callbackCount <= countBeforeStop + 1);
	}
}

TEST_CASE("CFileInterface - Empty files", "[fileInterface]")
{
	SECTION("Empty file handling")
	{
		TestContext ctx;
		SFileConfig config{
		    .path            = kTestDataDir / "empty.txt",
		    .isText          = false,
		    .chunkSize       = 10,
		    .sendingInterval = 50ms,
		    .loop            = false};

		CFileInterface fileInterface(ctx.ioc, config);

		auto dataCb = [&](std::span<const uint8_t> data)
		{
			ctx.receivedData.emplace_back(data.begin(), data.end());
			++ctx.callbackCount;
		};

		auto errorCb = [&](std::error_code ec)
		{
			ctx.errors.push_back(ec);
		};

		auto ec = fileInterface.start(dataCb, errorCb);
		REQUIRE(!ec);

		ctx.start();
		std::this_thread::sleep_for(150ms);
		fileInterface.stop();
		ctx.stop();

		// Empty file should not trigger data callbacks
		REQUIRE(ctx.callbackCount == 0);
	}
}
