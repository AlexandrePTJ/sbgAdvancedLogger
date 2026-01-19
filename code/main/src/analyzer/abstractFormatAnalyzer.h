// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Alexandre Petitjean <alpetitjean@gmail.com>
#pragma once

// Project headers
#include <common/types.h>

namespace sbg::advlogger
{
	/*!
     * \brief Abstract base class for format analyzers
     */
	class CAbstractFormatAnalyzer
	{
	public:
		//---------------------------------------------------------------------//
		//- Constructor / Destructor                                          -//
		//---------------------------------------------------------------------//
		CAbstractFormatAnalyzer()          = default;
		virtual ~CAbstractFormatAnalyzer() = default;

		//---------------------------------------------------------------------//
		//- Parameters                                                        -//
		//---------------------------------------------------------------------//
		/*!
		 * \brief Set callback for detected positions
		 */
		void setPositionCallback(PositionCallback positionCb);

		/*!
		 * \brief Allows to identify analyzer beyond others
		 */
		virtual std::string getId() const = 0;

		//---------------------------------------------------------------------//
		//- Operations                                                        -//
		//---------------------------------------------------------------------//
		/*!
         * \brief Process incoming data
         * \param data Raw data chunk
         */
		virtual void process(std::span<const uint8_t> data) = 0;

	protected:
		PositionCallback m_positionCb;
	};

}// namespace sbg::advlogger