// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Alexandre Petitjean <alpetitjean@gmail.com>
#pragma once

// 3rd party headers
#include <asio.hpp>

// project headers
#include <common/types.h>
#include <config/config.h>

namespace sbg::advlogger
{
	/*!
     * \brief Reusable TCP connection handler
     *
     * Provides basic TCP client functionality with async read/write.
     * Used by both TcpSource and NtripClient.
     */
	class CTcpConnection
	{
	public:
		//---------------------------------------------------------------------//
		//- Constructor / Destructor                                          -//
		//---------------------------------------------------------------------//
		explicit CTcpConnection(asio::io_context &ioc, const STcpConfig &tcpConfig, int32_t readBufferSize = 8192, int32_t sendBufferSize = 65536, int32_t receiveBufferSize = 65536);
		~CTcpConnection();

		// Non-copyable, non-movable
		CTcpConnection(const CTcpConnection &)            = delete;
		CTcpConnection &operator=(const CTcpConnection &) = delete;

		//---------------------------------------------------------------------//
		//- Operations                                                        -//
		//---------------------------------------------------------------------//
		/*!
         * \brief Asynchronously connect to the remote host
         * \param callback Called when connection completes or fails
         */
		void asyncConnect(const ConnectCallback &callback);

		/*!
         * \brief Start reading data from the socket
         * \param dataCb Called when data is received
         * \param errorCb Called on read errors
         */
		void startRead(DataCallback dataCb, ErrorCallback errorCb);

		/*!
         * \brief Stop reading (but keep the connection alive)
         */
		void stopRead();

		/*!
         * \brief Write data asynchronously
         * \param data Data to write
         * \param errorCb Called on errors
         */
		void asyncWrite(std::span<const uint8_t> data, const WriteCallback &writeCb);
		void asyncWrite(std::string_view data, const WriteCallback &writeCb);

		/*!
         * \brief Write data synchronously
         * \param data Data to write
         * \return Error code
         */
		std::error_code write(std::span<const uint8_t> data);
		std::error_code write(std::string_view data);

		/*!
         * \brief Close the connection
         */
		void close();

		/*!
         * \brief Check if connected
         */
		bool isConnected() const;

		/*!
         * \brief Get the underlying socket (for advanced usage)
         */
		asio::ip::tcp::socket &socket();

	private:
		//---------------------------------------------------------------------//
		//- Internal operations                                               -//
		//---------------------------------------------------------------------//
		void doRead();
		void handleReadError(const std::error_code &ec);

		asio::io_context &m_ioc;

		STcpConfig m_tcpConfig;

		int32_t m_sendBufferSize;
		int32_t m_receiveBufferSize;

		asio::ip::tcp::socket m_socket;
		std::vector<uint8_t>  m_readBuffer;
		DataCallback          m_dataCb;
		ErrorCallback         m_errorCb;
		bool                  m_isConnected = false;
		bool                  m_isReading   = false;
	};

}// namespace sbg::advlogger