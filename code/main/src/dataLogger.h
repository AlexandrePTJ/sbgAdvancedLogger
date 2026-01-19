// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Alexandre Petitjean <alpetitjean@gmail.com>
#pragma once

// 3rdparty headers
#include <asio.hpp>

// Project headers
#include <analyzer/abstractFormatAnalyzer.h>
#include <common/types.h>
#include <config/config.h>
#include <file/bufferedWriter.h>
#include <interfaces/abstractInterface.h>

namespace sbg::advlogger
{
	class CNtripClient;

	/*!
     * \brief Main datalogger application
     *
     * Coordinates all components:
     * - Device source (Serial/TCP/UDP)
     * - File writer with rotation
     * - Format analyzers (NMEA/sbgECom)
     * - NTRIP client (optional)
     * - RTCM forwarder (optional)
     */
	class CDataLogger
	{
	public:
		//---------------------------------------------------------------------//
		//- Constructor / Destructor                                          -//
		//---------------------------------------------------------------------//
		explicit CDataLogger(const CConfig &config);
		virtual ~CDataLogger();

		// Non-copyable, non-movable
		CDataLogger(const CDataLogger &)            = delete;
		CDataLogger &operator=(const CDataLogger &) = delete;

		//---------------------------------------------------------------------//
		//- Operations                                                        -//
		//---------------------------------------------------------------------//
		/*!
         * \brief Initialize all components
         * \return Error code, empty on success
         */
		std::error_code initialize(const std::filesystem::path &outputDirectory);

		/*!
         * \brief Run the datalogger (blocks until stopped)
         *
         * Starts the device source and runs the io_context event loop.
         * This call blocks until stop() is called or an error occurs.
         */
		void run();

		/*!
         * \brief Stop the datalogger
         *
         * Stops all components gracefully:
         * - Device source
         * - File writer (flushes remaining data)
         * - NTRIP client
         * - RTCM forwarder
         * - io_context event loop
         */
		void stop();

	private:
		//---------------------------------------------------------------------//
		//- Internal operations                                               -//
		//---------------------------------------------------------------------//
		std::error_code initializeSlot(const SSlotConfig &slotConfig, const std::filesystem::path &outputDirectory);
		std::error_code initializeRtcm(const SRtcmConfig &rtcmConfig, const std::filesystem::path &outputDirectory);
		void            initializeAnalyzers();
		void            onCriticalError(std::error_code errorCode);

		void onDeviceDataReceived(std::span<const uint8_t> data);
		void onRtcmDataReceived(std::span<const uint8_t> data);
		void onRtcmError(std::error_code errorCode);
		void onPositionDetected(const SNmeaGgaMessage &pos);

		//---------------------------------------------------------------------//
		//- Internal data                                                     -//
		//---------------------------------------------------------------------//
		CConfig m_config;
		bool    m_isRunning = false;

		asio::io_context m_ioc;

		std::unique_ptr<CAbstractInterface> m_deviceSlot;
		std::unique_ptr<CBufferedWriter>    m_deviceLogger;

		std::unique_ptr<CNtripClient>    m_ntripClient;
		std::unique_ptr<CBufferedWriter> m_ntripLogger;

		std::vector<std::unique_ptr<CAbstractFormatAnalyzer>> m_analyzers;
	};

}// namespace sbg::advlogger