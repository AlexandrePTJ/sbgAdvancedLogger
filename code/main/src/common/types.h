// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Alexandre Petitjean <alpetitjean@gmail.com>
#pragma once

// STL headers
#include <chrono>
#include <cstdint>
#include <functional>
#include <span>
#include <system_error>

namespace sbg::advlogger
{
	//---------------------------------------------------------------------//
	//- Data types                                                        -//
	//---------------------------------------------------------------------//

	/*!
	 * NMEA messages definitions
	 */
	enum class ENmeaGgaFixType : uint8_t
	{
		Invalid          = 0,
		Single           = 1,
		DGPS             = 2,
		RTKFixed         = 4,
		RTKFloat         = 5,
		INSDeadReckoning = 6,
	};

	struct SNmeaGgaMessage
	{
		std::chrono::system_clock::time_point timestamp;
		double                                latitude;
		double                                longitude;
		ENmeaGgaFixType                       gpsQuality            = ENmeaGgaFixType::Invalid;
		uint8_t                               numSatellites         = 0;
		double                                hdop                  = 0;
		double                                orthometricHeight     = 0;
		double                                ageOfDifferentialData = 0;
		uint16_t                              diffRefStationId      = 0;
	};

	//---------------------------------------------------------------------//
	//- Callbacks                                                         -//
	//---------------------------------------------------------------------//
	/*!
	 * \brief Callback for received data
	 * \param data Span of received bytes
	 */
	using DataCallback = std::function<void(std::span<const uint8_t> data)>;

	/*!
	 * \brief Callback for errors
	 * \param ec Error code describing the failure
	 */
	using ErrorCallback = std::function<void(std::error_code ec)>;

	/*!
	 * \brief Callback for connection requests
	 * \param ec Error code describing the failure
	 */
	using ConnectCallback = std::function<void(std::error_code ec)>;

	/*!
	 * \brief Callback for write operations
	 * \param ec Error code describing the failure
	 */
	using WriteCallback = std::function<void(std::error_code ec)>;

	using SourceTableCallback = std::function<void(std::string sourceTable)>;

	using RtcmCallback = std::function<void(std::span<const uint8_t> rtcmData)>;

	using PositionCallback = std::function<void(const SNmeaGgaMessage &pos)>;

}// namespace sbg::advlogger
