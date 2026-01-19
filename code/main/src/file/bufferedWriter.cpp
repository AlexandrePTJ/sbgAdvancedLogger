#include "bufferedWriter.h"

#include <spdlog/spdlog.h>

using namespace sbg::advlogger;

//---------------------------------------------------------------------//
//- Constructor / Destructor                                          -//
//---------------------------------------------------------------------//
CBufferedWriter::CBufferedWriter(asio::io_context &ioc):
m_ioc(ioc),
m_writeBuffer(m_bufferSize),
m_flushBuffer(m_bufferSize)
{
}

CBufferedWriter::~CBufferedWriter()
{
	stop();
}

//---------------------------------------------------------------------//
//- Parameters                                                        -//
//---------------------------------------------------------------------//
void CBufferedWriter::setFilePath(const std::filesystem::path &filepath)
{
	m_filePath = filepath;
}

void CBufferedWriter::setBufferSize(size_t size)
{
	if (!m_isActive)
	{
		m_bufferSize = size;

		m_writeBuffer.resize(size);
		m_flushBuffer.resize(size);
	}
}

bool CBufferedWriter::isActive() const
{
	return m_isActive;
}

//---------------------------------------------------------------------//
//- Operations                                                        -//
//---------------------------------------------------------------------//
bool CBufferedWriter::start()
{
	if (m_isActive)
	{
		spdlog::warn("BufferedWriter already active");
		return false;
	}

	try
	{
		// Create and open the file for async operations
		m_file = std::make_unique<asio::stream_file>(
		    m_ioc,
		    m_filePath.string(),
		    asio::stream_file::write_only | asio::stream_file::create | asio::stream_file::truncate);

		m_isActive        = true;
		m_writeBufferPos  = 0;
		m_flushBufferSize = 0;
		m_isWriting       = false;

		spdlog::info("BufferedWriter started: {}", m_filePath.string());
		return true;
	}
	catch (const std::exception &e)
	{
		spdlog::error("Failed to open file: {} - {}", m_filePath.string(), e.what());
		return false;
	}
}

void CBufferedWriter::write(std::span<const uint8_t> data)
{
	if (!m_isActive)
	{
		spdlog::warn("BufferedWriter not active, dropping {} bytes", data.size());
		return;
	}

	size_t totalWritten = 0;

	while (totalWritten < data.size())
	{
		size_t remaining = data.size() - totalWritten;
		size_t available = m_writeBuffer.size() - m_writeBufferPos;

		// If write buffer is full, try to swap
		if (available == 0)
		{
			// Check if async write is in progress or flush buffer has data
			if (m_isWriting || m_flushBufferSize > 0)
			{
				// Can't swap yet, data loss
				spdlog::error("Buffers full, dropping {} bytes", remaining);
				return;
			}

			// Swap buffers
			std::swap(m_writeBuffer, m_flushBuffer);
			m_flushBufferSize = m_writeBufferPos;
			m_writeBufferPos  = 0;

			// Start async write
			startAsyncWrite();

			available = m_writeBuffer.size();
		}

		// Write what fits in current buffer (partial write support)
		size_t toWrite = std::min(remaining, available);
		std::memcpy(m_writeBuffer.data() + m_writeBufferPos,
		            data.data() + totalWritten,
		            toWrite);
		m_writeBufferPos += toWrite;
		totalWritten += toWrite;

		// If write buffer is now full, trigger swap
		if (m_writeBufferPos >= m_writeBuffer.size())
		{
			if (!m_isWriting && m_flushBufferSize == 0)
			{
				std::swap(m_writeBuffer, m_flushBuffer);
				m_flushBufferSize = m_writeBufferPos;
				m_writeBufferPos  = 0;

				startAsyncWrite();
			}
		}
	}
}

void CBufferedWriter::startAsyncWrite()
{
	if (!m_file || !m_file->is_open() || m_flushBufferSize == 0)
	{
		return;
	}

	m_isWriting = true;

	asio::async_write(
	    *m_file,
	    asio::buffer(m_flushBuffer.data(), m_flushBufferSize),
	    [this](const asio::error_code &ec, size_t bytesWritten)
	    {
		    handleWriteComplete(ec, bytesWritten);
	    });
}

void CBufferedWriter::handleWriteComplete(const asio::error_code &ec, size_t bytesWritten)
{
	m_isWriting = false;

	if (ec)
	{
		spdlog::error("File write error: {}", ec.message());
		return;
	}

	spdlog::trace("Wrote {} bytes to file", bytesWritten);

	// Mark flush buffer as empty
	m_flushBufferSize = 0;

	// If write buffer has pending data and we can swap, do it
	if (m_writeBufferPos > 0 && !m_isWriting)
	{
		std::swap(m_writeBuffer, m_flushBuffer);
		m_flushBufferSize = m_writeBufferPos;
		m_writeBufferPos  = 0;

		// Continue writing
		startAsyncWrite();
	}
}

void CBufferedWriter::stop()
{
	if (!m_isActive)
	{
		return;
	}

	// Flush any remaining data in write buffer
	if (m_writeBufferPos > 0 && !m_isWriting && m_flushBufferSize == 0)
	{
		std::swap(m_writeBuffer, m_flushBuffer);
		m_flushBufferSize = m_writeBufferPos;
		m_writeBufferPos  = 0;

		startAsyncWrite();
	}

	// Wait for pending async operations to complete
	// This is done by running the io_context until all handlers complete
	// Note: In a real application, you might want to use a more sophisticated
	// synchronization mechanism or a timer to avoid blocking indefinitely
	while (m_isWriting)
	{
		m_ioc.poll_one();
	}

	// Final flush if there's still data
	if (m_flushBufferSize > 0 && m_file && m_file->is_open())
	{
		std::error_code ec;
		asio::write(*m_file, asio::buffer(m_flushBuffer.data(), m_flushBufferSize), ec);
		if (ec)
		{
			spdlog::error("Final flush error: {}", ec.message());
		}
		m_flushBufferSize = 0;
	}

	// Close file
	if (m_file && m_file->is_open())
	{
		std::error_code ec;
		m_file->close(ec);
		if (ec)
		{
			spdlog::error("File close error: {}", ec.message());
		}
	}

	m_file.reset();
	m_isActive = false;

	spdlog::info("BufferedWriter stopped");
}
