// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: 2026 Alexandre Petitjean <alpetitjean@gmail.com>
#pragma once

// 3rdparty headers
#include <asio.hpp>

// STL headers
#include <filesystem>
#include <memory>
#include <span>
#include <vector>

namespace sbg::advlogger
{
	/*!
     * \brief Asynchronous double-buffered file writer
     *
     * Uses two buffers and Asio's async file I/O to write data without blocking.
     * Integrates seamlessly with the application's io_context event loop.
     * Supports partial writes when buffers are nearly full.
     */
	class CBufferedWriter
	{
	public:
		//---------------------------------------------------------------------//
		//- Constructor / Destructor                                          -//
		//---------------------------------------------------------------------//
		explicit CBufferedWriter(asio::io_context &ioc);
		~CBufferedWriter();

		// Non-copyable, non-movable
		CBufferedWriter(const CBufferedWriter &)            = delete;
		CBufferedWriter &operator=(const CBufferedWriter &) = delete;

		//---------------------------------------------------------------------//
		//- Parameters                                                        -//
		//---------------------------------------------------------------------//
		void setFilePath(const std::filesystem::path &filepath);
		void setBufferSize(size_t size);
		bool isActive() const;

		//---------------------------------------------------------------------//
		//- Operations                                                        -//
		//---------------------------------------------------------------------//

		/*!
         * \brief Start the writer with the given file path
         * \return true on success, false on error
         */
		bool start();

		/*!
         * \brief Write data to the buffer (non-blocking)
         * \param data Data to write
         *
         * If the current buffer doesn't have enough space, writes what fits
         * and triggers an async flush. May drop data if both buffers are full.
         */
		void write(std::span<const uint8_t> data);

		/*!
         * \brief Stop the writer and flush remaining data
         */
		void stop();

	private:
		void startAsyncWrite();
		void handleWriteComplete(const asio::error_code &ec, size_t bytesWritten);

		asio::io_context &m_ioc;

		// Asio file handle for async operations
		std::filesystem::path m_filePath;
		std::unique_ptr<asio::stream_file> m_file;

		// Double buffering
		size_t               m_bufferSize = 4096;// 1MB per buffer
		std::vector<uint8_t> m_writeBuffer;
		std::vector<uint8_t> m_flushBuffer;
		size_t               m_writeBufferPos  = 0;
		size_t               m_flushBufferSize = 0;

		bool m_isActive  = false;
		bool m_isWriting = false;// Async write in progress
	};

}// namespace sbg::advlogger
