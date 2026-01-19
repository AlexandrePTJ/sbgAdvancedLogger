#include "tcpInterface.h"

using namespace sbg::advlogger;

//---------------------------------------------------------------------//
//- Constructor / Destructor                                          -//
//---------------------------------------------------------------------//
CTcpInterface::CTcpInterface(asio::io_context &ioc, const STcpConfig &tcpConfig):
CAbstractInterface(ioc),
m_tcpConnection(ioc, tcpConfig)
{
}

CTcpInterface::~CTcpInterface() = default;

//---------------------------------------------------------------------//
//- Operations                                                        -//
//---------------------------------------------------------------------//
std::error_code CTcpInterface::start(DataCallback dataCb, ErrorCallback errorCb)
{
	std::error_code result;
	bool            connectFinished = false;

	// clang-format off
	m_tcpConnection.asyncConnect([&](std::error_code errorCode)
	{
		result = errorCode;
		connectFinished = true;

        if (!errorCode)
		{
			m_tcpConnection.startRead(std::move(dataCb), std::move(errorCb));
        }
	});
	// clang-format on

	// Run io_context until the connection completes
	while (!connectFinished)
	{
		m_ioc.poll_one();
	}

	return result;
}

void CTcpInterface::stop()
{
	m_tcpConnection.close();
}

void CTcpInterface::asyncWrite(std::span<const uint8_t> data, ErrorCallback errorCb)
{
	if (m_tcpConnection.isConnected())
	{
		m_tcpConnection.asyncWrite(data, std::move(errorCb));
	}
	else if (errorCb)
	{
		errorCb(std::make_error_code(std::errc::not_connected));
	}
}

bool CTcpInterface::isActive() const
{
	return m_tcpConnection.isConnected();
}
