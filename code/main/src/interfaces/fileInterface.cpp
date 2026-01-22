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

	m_file = std::make_unique<asio::stream_file>(m_ioc, m_fileConfig.path.string(), asio::stream_file::read_only);

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
			doRead();
			scheduleRead();
		}
	});
	// clang-format on
}

void CFileInterface::doRead()
{
}
