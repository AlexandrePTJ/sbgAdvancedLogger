// STL headers
#include <chrono>
#include <csignal>

// 3rdparty headers
#include <CLI/CLI.hpp>
#include <fmt/chrono.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

// Project headers
#include "config/config.h"
#include "dataLogger.h"

namespace
{
	std::unique_ptr<sbg::advlogger::CDataLogger> gLogger;

	void signalHandler(int signal)
	{
		spdlog::info("Received signal {}, shutting down...", signal);
		if (gLogger)
		{
			gLogger->stop();
		}
	}
}// namespace

int main(int argc, char *argv[])
{
	try
	{
		// CLI setup
		CLI::App app{"sbgAdvancedLogger - High-performance data acquisition tool"};

		std::filesystem::path configPath = "config.json";
		app.add_option("-c,--config", configPath, "Path to configuration file")
		    ->check(CLI::ExistingFile);

		std::filesystem::path outputDirectory = "output";
		app.add_option("-o,--output", outputDirectory, "Output directory")
		    ->check(CLI::ExistingDirectory);

		CLI11_PARSE(app, argc, argv);

		// Setup signal handlers
		std::signal(SIGINT, signalHandler);
		std::signal(SIGTERM, signalHandler);
		std::signal(SIGABRT, signalHandler);

		// Load configuration
		auto config = sbg::advlogger::CConfig::loadFromJsonFile(configPath);

		// Create 2 loggers : general default logger to console, events logger to file in output dir
		auto consoleLogger = spdlog::stdout_color_mt("console");
		spdlog::set_default_logger(consoleLogger);

		auto eventLogger = spdlog::basic_logger_mt("events", (outputDirectory / fmt::format("events_{:%Y%m%d_%H%M%S}.log", std::chrono::system_clock::now())).string());

		// Startup : Create datalogger instance, validate config and check source / targets
		spdlog::info("sbgAdvancedLogger starting...");
		spdlog::info("Configuration loaded from: {}", configPath.string());

		// Create data logger instance
		gLogger = std::make_unique<sbg::advlogger::CDataLogger>(config);
		if (auto errorCode = gLogger->initialize(outputDirectory))
		{
			spdlog::error("Failed to initialize data logger: {}", errorCode.message());
			return 1;
		}

		// Run to the hills
		spdlog::info("------ Press Ctrl+C to stop ------");

		gLogger->run();

		spdlog::info("DataLogger stopped cleanly");
	}
	catch (std::exception &e)
	{
		spdlog::error("Exception: {}", e.what());
		return 1;
	}

	return 0;
}
