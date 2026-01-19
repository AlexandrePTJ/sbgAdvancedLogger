// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Alexandre Petitjean <alpetitjean@gmail.com>
#pragma once

// STL headers
#include <string>

// Project headers
#include <common/types.h>

namespace sbg::advlogger
{
	/*!
	 * NMEA helpers
	 */
	class CNmeaFactory
	{
	public:
		//---------------------------------------------------------------------//
		//- Operations                                                        -//
		//---------------------------------------------------------------------//
		static SNmeaGgaMessage createGGAFromSimplePosition(double latitude, double longitude);
	};

	/*!
	 * NMEA message encoder
	 */
	class CNmeaEncoder
	{
	public:
		//---------------------------------------------------------------------//
		//- Operations                                                        -//
		//---------------------------------------------------------------------//
		static std::string encode(const SNmeaGgaMessage &ggaMessage);
	};

}// namespace sbg::advlogger