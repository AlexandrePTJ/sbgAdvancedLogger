#include "dataLogger.h"

// 3rdparty headers
#include <spdlog/spdlog.h>

// Project headers
#include <analyzer/sbgEComAnalyzer.h>
#include <common/nmea.h>
#include <interfaces/fileInterface.h>
#include <interfaces/udpInterface.h>
#include <ntrip/ntripClient.h>

using namespace sbg::advlogger;

//---------------------------------------------------------------------//
//- Constructor / Destructor                                          -//
//---------------------------------------------------------------------//
CDataLogger::CDataLogger(const CConfig &config):
m_config(config)
{
}

CDataLogger::~CDataLogger()
{
	stop();
}

//---------------------------------------------------------------------//
//- Operations                                                        -//
//---------------------------------------------------------------------//
std::error_code CDataLogger::initialize(const std::filesystem::path &outputDirectory)
{
	spdlog::info("Initializing DataLogger...");

	if (m_config.device.has_value())
	{
		if (auto errorCode = initializeSlot(m_config.device.value(), outputDirectory))
		{
			return errorCode;
		}
	}

	if (m_config.rtcm.has_value())
	{
		if (auto errorCode = initializeRtcm(m_config.rtcm.value(), outputDirectory))
		{
			return errorCode;
		}
	}

	initializeAnalyzers();

	spdlog::info("DataLogger initialized successfully");
	return {};
}

void CDataLogger::run()
{
	if (m_isRunning)
	{
		spdlog::warn("DataLogger already running");
		return;
	}

	if (m_deviceSlot)
	{
		auto deviceStartErrorCode = m_deviceSlot->start([this](const auto &data)
		                                                { onDeviceDataReceived(data); },
		                                                [this](const auto &errorCode)
		                                                { onCriticalError(errorCode); });

		if (deviceStartErrorCode)
		{
			spdlog::error("Failed to start slot : {}", deviceStartErrorCode.message());
			return;
		}

		if (m_deviceLogger)
		{
			m_deviceLogger->start();
		}
	}

	if (m_ntripClient)
	{
		if (auto errorCode = m_ntripClient->connect())
		{
			spdlog::error("Failed to connect to NTRIP server: {}", errorCode.message());
			return;
		}

		m_ntripClient->startListenMountPoint([this](const auto &data)
		                                     { onRtcmDataReceived(data); },
		                                     [this](const auto &errorCode)
		                                     { onRtcmError(errorCode); });

		if (m_ntripLogger)
		{
			m_ntripLogger->start();
		}
	}

	m_isRunning = true;

	// Run io_context (blocks here)
	auto work = asio::make_work_guard(m_ioc);
	m_ioc.restart();
	m_ioc.run();

	// Stop
	spdlog::info("DataLogger stopped");
	m_isRunning = false;
}

void CDataLogger::stop()
{
	if (!m_isRunning)
	{
		return;
	}

	spdlog::info("Stopping DataLogger...");

	if (m_ntripClient)
	{
		m_ntripClient->disconnect();

		if (m_ntripLogger)
		{
			m_ntripLogger->stop();
		}
	}

	m_ioc.stop();
	m_isRunning = false;
}

//---------------------------------------------------------------------//
//- Internal operations                                               -//
//---------------------------------------------------------------------//
std::error_code CDataLogger::initializeSlot(const SSlotConfig &slotConfig, const std::filesystem::path &outputDirectory)
{
	spdlog::info("Setting up device...");
	if (slotConfig.deviceConfig.is<SUdpConfig>())
	{
		spdlog::info("=== UDP ===");
		m_deviceSlot = std::make_unique<CUdpInterface>(m_ioc, slotConfig.deviceConfig.get<SUdpConfig>());
	}
	else if (slotConfig.deviceConfig.is<SFileConfig>())
	{
		spdlog::info("=== File ===");
		m_deviceSlot = std::make_unique<CFileInterface>(m_ioc, slotConfig.deviceConfig.get<SFileConfig>());
	}
	else
	{
		spdlog::error("Unsupported device configuration");
		return std::make_error_code(std::errc::operation_not_supported);
	}

	if (slotConfig.outputFile.has_value())
	{
		spdlog::info("=== Output ===");
		m_deviceLogger = std::make_unique<CBufferedWriter>(m_ioc);
		m_deviceLogger->setFilePath(outputDirectory / "device.bin");
	}

	return {};
}

std::error_code CDataLogger::initializeRtcm(const SRtcmConfig &rtcmConfig, const std::filesystem::path &outputDirectory)
{
	spdlog::info("Setting up RTCM...");
	if (rtcmConfig.config.is<SNtripConfig>())
	{
		const auto &ntripConfig = rtcmConfig.config.get<SNtripConfig>();

		spdlog::info("=== NTRIP ===");
		spdlog::info("Host: {}:{}, MountPoint: {}", ntripConfig.host, ntripConfig.port, ntripConfig.mountpoint);
		m_ntripClient = std::make_unique<CNtripClient>(m_ioc, ntripConfig);
	}
	else if (rtcmConfig.config.is<SSerialConfig>())
	{
		spdlog::info("=== Serial ===");
		spdlog::error("Serial RTCM forwarding not yet implemented");
		return std::make_error_code(std::errc::operation_not_supported);
	}

	if (rtcmConfig.inputConfig.is<SFixedPositionConfig>() && m_ntripClient)
	{
		const auto &positionConfig = rtcmConfig.inputConfig.get<SFixedPositionConfig>();

		auto ggaMsg   = CNmeaFactory::createGGAFromSimplePosition(positionConfig.latitude, positionConfig.longitude);
		auto ggaFrame = CNmeaEncoder::encode(ggaMsg);

		m_ntripClient->setGgaFrame(ggaFrame);
	}

	if (rtcmConfig.outputFile.has_value())
	{
		spdlog::info("=== Output ===");
		m_ntripLogger = std::make_unique<CBufferedWriter>(m_ioc);
		m_ntripLogger->setFilePath(outputDirectory / "rtcm.bin");
	}

	return {};
}

void CDataLogger::initializeAnalyzers()
{
	{
		auto analyzer = std::make_unique<CSbgEComAnalyzer>();
		analyzer->setPositionCallback([this](const auto &pos)
		                              { onPositionDetected(pos); });
		m_analyzers.emplace_back(std::move(analyzer));
	}
}

void CDataLogger::onCriticalError(std::error_code errorCode)
{
	if (errorCode)
	{
		spdlog::error("Critical error: {}", errorCode.message());
		stop();
	}
}

void CDataLogger::onDeviceDataReceived(std::span<const uint8_t> data)
{
	if (m_deviceLogger)
	{
		m_deviceLogger->write(data);
	}

	for (auto &analyzer : m_analyzers)
	{
		analyzer->process(data);
	}
}

void CDataLogger::onRtcmDataReceived(std::span<const uint8_t> data)
{
	if (m_deviceSlot)
	{
		m_deviceSlot->asyncWrite(data, [](std::error_code errorCode)
		                         {
			if (errorCode)
			{
				spdlog::error("Failed to write RTCM data to device: {}", errorCode.message());
			} });
	}

	if (m_ntripLogger)
	{
		m_ntripLogger->write(data);
	}
}

void CDataLogger::onRtcmError(std::error_code errorCode)
{
	onCriticalError(errorCode);
}

void CDataLogger::onPositionDetected(const SNmeaGgaMessage &pos)
{
	if (m_ntripClient)
	{
		m_ntripClient->setGgaFrame(CNmeaEncoder::encode(pos));
	}
}

//
// void DataLogger::on_device_data(std::span<const uint8_t> data)
// {
//     // 1. Write to file (CRITICAL - no data loss)
//     if (!file_rotator_->write(data))
//     {
//         spdlog::error("Failed to write data to file - POTENTIAL DATA LOSS!");
//     }
//
//     // 2. Feed to analyzers (for position detection)
//     for (auto &analyzer : analyzers_)
//     {
//         analyzer->process(data);
//     }
// }
//
// void DataLogger::on_device_error(std::error_code ec)
// {
//     spdlog::error("Device error: {} - stopping datalogger", ec.message());
//     stop();
// }
//
// void DataLogger::on_position_detected(const analyzer::PositionData &pos)
// {
//     spdlog::debug("Position detected: {:.6f}, {:.6f}, alt={:.1f}m, sats={}",
//                   pos.latitude, pos.longitude, pos.altitude, pos.satellites);
//
//     // Send position to NTRIP caster if enabled
//     if (ntrip_client_ && ntrip_client_->is_connected())
//     {
//         ntrip_client_->send_position(pos);
//     }
// }
//
// void DataLogger::on_rtcm_received(std::span<const uint8_t> rtcm_data)
// {
//     spdlog::trace("RTCM data received: {} bytes", rtcm_data.size());
//
//     // Forward RTCM data to configured destination
//     if (rtcm_forwarder_)
//     {
//         rtcm_forwarder_->forward(rtcm_data);
//     }
// }
