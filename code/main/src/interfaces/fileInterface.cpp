#include "fileInterface.h"

#include <spdlog/spdlog.h>

using namespace sbg::advlogger;

//---------------------------------------------------------------------//
//- Constructor / Destructor                                          -//
//---------------------------------------------------------------------//
CFileInterface::CFileInterface(asio::io_context &ioc, const SFileConfig &config):
CAbstractInterface(ioc),
m_fileConfig(config),
m_readTimer(ioc)
{
}

CFileInterface::~CFileInterface()
{
	CFileInterface::stop();
}

//---------------------------------------------------------------------//
//- Operations                                                        -//
//---------------------------------------------------------------------//
std::error_code CFileInterface::start(DataCallback dataCb, ErrorCallback errorCb)
{
	m_dataCb  = std::move(dataCb);
	m_errorCb = std::move(errorCb);

	if (!std::filesystem::exists(m_fileConfig.path) || !std::filesystem::is_regular_file(m_fileConfig.path))
	{
		return std::make_error_code(std::errc::invalid_argument);
	}

	if (m_fileConfig.chunkSize == 0 && (!m_fileConfig.isText || !m_fileConfig.textChunkToNewLine))
	{
		return std::make_error_code(std::errc::invalid_argument);
	}

	m_file = std::make_unique<asio::stream_file>(m_ioc, m_fileConfig.path.string(), asio::stream_file::read_only);

	if (m_fileConfig.isText && m_fileConfig.textChunkToNewLine)
	{
		m_readFunc = [this]
		{ doReadLine(); };
	}
	else
	{
		m_readFunc = [this]
		{ doReadChunk(); };
	}

	m_isRunning = true;
	scheduleRead();

	return {};
}

void CFileInterface::stop()
{
	m_isRunning = false;
}

void CFileInterface::asyncWrite(std::span<const uint8_t> /*data*/, ErrorCallback /*errorCb*/)
{
	// Don't overwrite file, nothing to do here
}

//---------------------------------------------------------------------//
//- Internal operations                                               -//
//---------------------------------------------------------------------//
void CFileInterface::scheduleRead()
{
	m_readTimer.expires_after(m_fileConfig.sendingInterval);

	// clang-format off
	m_readTimer.async_wait([this](std::error_code ec)
	{
		if (!ec && m_isRunning)
		{
			m_readFunc();
			scheduleRead();
		}
	});
	// clang-format on
}

void CFileInterface::doReadLine()
{
	// clang-format off
	asio::async_read_until(*m_file, m_lineBuffer, '\n', [this](std::error_code ec, std::size_t bytesTransferred)
	{
		if (!ec)
		{
			std::string line(asio::buffers_begin(m_lineBuffer.data()), asio::buffers_begin(m_lineBuffer.data()) + bytesTransferred);
			m_lineBuffer.consume(bytesTransferred);

			m_dataCb({reinterpret_cast<const uint8_t*>(line.data()), line.size()});
		}
		else if (ec == asio::error::eof && m_fileConfig.loop)
		{
			m_file->seek(0, asio::stream_file::seek_set);
		}
		else if (ec != asio::error::eof)
		{
			m_errorCb(ec);
		}
	});
	// clang-format on
}

void CFileInterface::doReadChunk()
{
	auto buffer = std::make_shared<std::vector<uint8_t>>(m_fileConfig.chunkSize);

	// clang-format off
	asio::async_read(*m_file, asio::buffer(*buffer), [this, buffer](std::error_code ec, std::size_t bytesTransferred)
	{
		if (!ec || (ec == asio::error::eof && bytesTransferred > 0))
		{
			m_dataCb(std::span<const uint8_t>(buffer->data(), bytesTransferred));

			auto atEof = m_file->size() == m_file->seek(0, asio::stream_file::seek_cur);
			if ((ec == asio::error::eof || atEof) && m_fileConfig.loop)
			{
				m_file->seek(0, asio::stream_file::seek_set);
			}
		}
		else if (ec == asio::error::eof && m_fileConfig.loop)
		{
			m_file->seek(0, asio::stream_file::seek_set);
		}
		else if (ec != asio::error::eof)
		{
			m_errorCb(ec);
		}
	});
	// clang-format on
}
