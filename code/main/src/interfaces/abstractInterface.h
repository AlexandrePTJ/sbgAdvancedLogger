// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Alexandre Petitjean <alpetitjean@gmail.com>
#pragma once

// 3rdparty headers
#include <asio.hpp>

// Project headers
#include <common/types.h>

namespace sbg::advlogger
{
	class CAbstractInterface
	{
	public:
		//---------------------------------------------------------------------//
		//- Constructor / Destructor                                          -//
		//---------------------------------------------------------------------//
		CAbstractInterface(asio::io_context &ioc);
		virtual ~CAbstractInterface() = default;

		//---------------------------------------------------------------------//
		//- Parameters                                                        -//
		//---------------------------------------------------------------------//

		//---------------------------------------------------------------------//
		//- Operations                                                        -//
		//---------------------------------------------------------------------//

		/*!
         * \brief Start receiving data from the device
         * \param data_cb Callback invoked when data is received
         * \param error_cb Callback invoked on errors
         * \return Error code, empty on success
         */
		virtual std::error_code start(DataCallback data_cb, ErrorCallback error_cb) = 0;

		/*!
         * \brief Stop receiving data
         */
		virtual void stop() = 0;

		/*!
         * \brief Send data to the device (for RTCM forwarding)
         * \param data Data to send
         * \param handler Completion handler
         */
		virtual void asyncWrite(std::span<const uint8_t> data, std::function<void(std::error_code)> handler) = 0;

		/*!
         * \brief Check if the source is currently active
         */
		virtual bool isActive() const;

	protected:
		//---------------------------------------------------------------------//
		//- Internal data                                                     -//
		//---------------------------------------------------------------------//
		asio::io_context &m_ioc;
		bool              m_isActive = false;
	};

}// namespace sbg::advlogger