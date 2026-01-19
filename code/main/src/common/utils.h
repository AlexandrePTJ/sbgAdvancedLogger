// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Alexandre Petitjean <alpetitjean@gmail.com>
#pragma once

// STL headers
#include <string>

namespace sbg::advlogger
{
	/*!
	 * \brief	Encode string as base64
	 * \param	input
	 * \return
	 */
	std::string encodeBase64(const std::string &input);

	/*!
	 * \brief			Compute checksum for NMEA messages
	 * \param	data	NMEA frame
	 * \return			CRC
	 */
	std::string computeXORCrc(const std::string &data);
}// namespace sbg::advlogger
