#include "config.h"

// STL headers
#include <fstream>

// 3rdparty headers
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

using namespace sbg::advlogger;

//---------------------------------------------------------------------//
//- Helpers                                                           -//
//---------------------------------------------------------------------//
namespace
{
	STcpConfig getTcpConfigFromJson(const nlohmann::json &value)
	{
		return {.host = value.at("host").get<std::string>(), .port = value.at("port").get<uint16_t>()};
	}

	bool hasTcpConfig(const nlohmann::json &value)
	{
		return value.contains("host") && value.contains("port");
	}

	SUdpConfig getUdpConfigFromJson(const nlohmann::json &value)
	{
		return {.host = value.at("host").get<std::string>(), .portIn = value.at("portIn").get<uint16_t>(), .portOut = value.at("portOut").get<uint16_t>()};
	}

	bool hasUdpConfig(const nlohmann::json &value)
	{
		return value.contains("host") && value.contains("portIn") && value.contains("portOut");
	}

	SSerialConfig getSerialConfigFromJson(const nlohmann::json &value)
	{
		return {.port = value.at("port").get<std::string>(), .baudRate = value.at("baudRate").get<uint32_t>()};
	}

	bool hasSerialConfig(const nlohmann::json &value)
	{
		return value.contains("port") && value.contains("baudRate");
	}

	SFileConfig getFileConfigFromJson(const nlohmann::json &value)
	{
		SFileConfig fileConfig;

		fileConfig.path               = value.at("path").get<std::string>();
		fileConfig.isText             = value.contains("isText") ? value.at("isText").get<bool>() : false;
		fileConfig.textChunkToNewLine = value.contains("textChunkToNewLine") ? value.at("textChunkToNewLine").get<bool>() : false;
		fileConfig.chunkSize          = value.contains("chunkSize") ? value.at("chunkSize").get<uint32_t>() : 0;
		fileConfig.loop               = value.contains("loop") ? value.at("loop").get<bool>() : false;

		if (value.contains("sendingInterval"))
		{
			fileConfig.sendingInterval = std::chrono::milliseconds(value.at("sendingInterval").get<uint32_t>());
		}

		return fileConfig;
	}

	bool hasFileConfig(const nlohmann::json &value)
	{
		return value.contains("path");
	}

	SNtripConfig getNtripConfigFromJson(const nlohmann::json &value)
	{
		SNtripConfig ntripConfig;

		ntripConfig.host       = value.at("host").get<std::string>();
		ntripConfig.port       = value.at("port").get<uint16_t>();
		ntripConfig.mountpoint = value.at("mountpoint").get<std::string>();
		ntripConfig.username   = value.contains("username") ? value.at("username").get<std::string>() : "";
		ntripConfig.password   = value.contains("password") ? value.at("password").get<std::string>() : "";

		if (value.contains("ggaSendingInterval"))
		{
			ntripConfig.ggaSendingInterval = std::chrono::seconds(value.at("ggaSendingInterval").get<uint32_t>());
		}

		return ntripConfig;
	}

	bool hasNtripConfig(const nlohmann::json &value)
	{
		return value.contains("host") && value.contains("port") && value.contains("mountpoint");
	}

	SFixedPositionConfig getFixedPositionConfigFromJson(const nlohmann::json &value)
	{
		return {.latitude = value.at("latitude").get<double>(), .longitude = value.at("longitude").get<double>()};
	}

	bool hasFixedPositionConfig(const nlohmann::json &value)
	{
		return value.contains("latitude") && value.contains("longitude");
	}

	std::variant<STcpConfig, SUdpConfig, SSerialConfig, SFileConfig> getDeviceConfigFromJson(const nlohmann::json &value)
	{
		if (hasTcpConfig(value))
		{
			return getTcpConfigFromJson(value);
		}
		if (hasUdpConfig(value))
		{
			return getUdpConfigFromJson(value);
		}
		if (hasSerialConfig(value))
		{
			return getSerialConfigFromJson(value);
		}
		if (hasFileConfig(value))
		{
			return getFileConfigFromJson(value);
		}
		throw std::runtime_error("Invalid device config");
	}

	std::variant<SNtripConfig, SSerialConfig> getRtcmDeviceConfigFromJson(const nlohmann::json &value)
	{
		if (hasSerialConfig(value))
		{
			return getSerialConfigFromJson(value);
		}
		if (hasNtripConfig(value))
		{
			return getNtripConfigFromJson(value);
		}
		throw std::runtime_error("Invalid RTCM device config");
	}

	std::variant<SFixedPositionConfig, std::string> getRtcmInputConfigFromJson(const nlohmann::json &value)
	{
		if (value.is_object() && hasFixedPositionConfig(value))
		{
			return getFixedPositionConfigFromJson(value);
		}
		if (value.is_string())
		{
			return value.get<std::string>();
		}
		throw std::runtime_error("Invalid RTCM input config");
	}

	SOutputFileConfig getOutputFileConfigFromJson(const nlohmann::json &value)
	{
		SOutputFileConfig outputFileConfig;

		outputFileConfig.filenamePrefix = value.at("filenamePrefix").get<std::string>();

		if (value.contains("splitInterval"))
		{
			auto splitInterval             = value.at("splitInterval").get<uint32_t>();
			outputFileConfig.splitInterval = std::chrono::seconds(splitInterval);
		}

		return outputFileConfig;
	}

	SSlotConfig getSlotConfigFromJson(const nlohmann::json &value)
	{
		SSlotConfig slotConfig;

		slotConfig.deviceConfig = getDeviceConfigFromJson(value.at("deviceConfig"));

		if (value.contains("outputFile"))
		{
			slotConfig.outputFile = getOutputFileConfigFromJson(value.at("outputFile"));
		}
		if (value.contains("enableRtcmForwarder"))
		{
			slotConfig.enableRtcmForwarder = value.at("enableRtcmForwarder").get<bool>();
		}

		return slotConfig;
	}

	SRtcmConfig getRtcmConfigFromJson(const nlohmann::json &value)
	{
		SRtcmConfig rtcmConfig;

		rtcmConfig.config      = getRtcmDeviceConfigFromJson(value.at("config"));
		rtcmConfig.inputConfig = getRtcmInputConfigFromJson(value.at("inputConfig"));

		if (value.contains("outputFile"))
		{
			rtcmConfig.outputFile = getOutputFileConfigFromJson(value.at("outputFile"));
		}

		return rtcmConfig;
	}
}// namespace

//---------------------------------------------------------------------//
//- Operations                                                        -//
//---------------------------------------------------------------------//
std::string STcpConfig::toString() const
{
	return fmt::format("{}:{}", host, port);
}

CConfig CConfig::loadFromJsonFile(const std::filesystem::path &jsonFilePath)
{
	try
	{
		std::ifstream file(jsonFilePath);
		if (!file.is_open())
		{
			throw std::runtime_error(fmt::format("Cannot open config file: {}", jsonFilePath.string()));
		}

		nlohmann::json json;
		file >> json;

		CConfig config;
		if (json.contains("device"))
		{
			config.device = getSlotConfigFromJson(json.at("device"));
		}
		if (json.contains("rtcm"))
		{
			config.rtcm = getRtcmConfigFromJson(json.at("rtcm"));
		}

		if (!config.device.has_value() && !config.rtcm.has_value())
		{
			throw std::runtime_error("No device or rtcm slot configured");
		}

		return config;
	}
	catch (const nlohmann::json::exception &e)
	{
		throw std::runtime_error(fmt::format("JSON parsing error: {}", e.what()));
	}
}
