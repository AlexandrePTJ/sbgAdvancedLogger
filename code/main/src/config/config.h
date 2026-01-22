// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Alexandre Petitjean <alpetitjean@gmail.com>
#pragma once

// STL headers
#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <variant>

namespace sbg::advlogger
{
	//---------------------------------------------------------------------//
	//- Definitions                                                       -//
	//---------------------------------------------------------------------//
	template<class... T>
	struct VariantConfigHelper
	{
		template<class C>
		bool is() const
		{
			return std::holds_alternative<C>(m_config);
		}

		template<class C>
		const C &get() const
		{
			return std::get<C>(m_config);
		}

		VariantConfigHelper &operator=(std::variant<T...> config)
		{
			m_config = config;
			return *this;
		}

	private:
		std::variant<T...> m_config;
	};

	struct STcpConfig
	{
		std::string host;
		uint16_t    port;

		std::string toString() const;
	};

	struct SUdpConfig
	{
		std::string host;
		uint16_t    portIn;
		uint16_t    portOut;
	};

	struct SSerialConfig
	{
		std::string port;
		uint32_t    baudRate;
		uint8_t     dataBits    = 8;
		std::string parity      = "none";// "none", "odd", "even"
		std::string stopBits    = "one"; // "one", "two"
		std::string flowControl = "none";// "none", "hardware", "software"
	};

	struct SFileConfig
	{
		std::filesystem::path     path;
		bool                      isText             = false;
		bool                      textChunkToNewLine = false;
		uint32_t                  chunkSize          = 0;
		std::chrono::milliseconds sendingInterval    = std::chrono::milliseconds{1000};
		bool                      loop               = false;
	};

	struct SNtripConfig
	{
		std::string          host;
		uint16_t             port;
		std::string          mountpoint;
		std::string          username;
		std::string          password;
		std::chrono::seconds ggaSendingInterval = std::chrono::seconds{1};
	};

	struct SFixedPositionConfig
	{
		double latitude;
		double longitude;
	};

	struct SOutputFileConfig
	{
		std::string                         filenamePrefix;
		std::optional<std::chrono::seconds> splitInterval = std::nullopt;
	};

	struct SSlotConfig
	{
		VariantConfigHelper<STcpConfig, SUdpConfig, SSerialConfig, SFileConfig> deviceConfig;
		std::optional<SOutputFileConfig>                                        outputFile          = std::nullopt;
		bool                                                                    enableRtcmForwarder = false;
	};

	struct SRtcmConfig
	{
		VariantConfigHelper<SNtripConfig, SSerialConfig>       config;
		std::optional<SOutputFileConfig>                       outputFile = std::nullopt;
		VariantConfigHelper<SFixedPositionConfig, std::string> inputConfig;
	};

	struct CConfig
	{
		std::optional<SSlotConfig> device = std::nullopt;
		std::optional<SRtcmConfig> rtcm   = std::nullopt;

		/*!
         * \brief Load configuration from the JSON file
         * \param jsonFilePath Path to config.json
         * \return Config object if successful
         */
		static CConfig loadFromJsonFile(const std::filesystem::path &jsonFilePath);
	};
}// namespace sbg::advlogger
