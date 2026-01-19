#include "httpGetContext.h"

// Local headers
#include "tcpConnection.h"

using namespace sbg::advlogger;

//---------------------------------------------------------------------//
//- Constructor / Destructor                                          -//
//---------------------------------------------------------------------//
CHttpGetContext::CHttpGetContext(asio::io_context &ioc, const std::chrono::seconds &timeout):
m_timeoutTimer(ioc)
{
	m_timeoutTimer.expires_after(timeout);
}

CHttpGetContext::~CHttpGetContext() = default;

//---------------------------------------------------------------------//
//- Parameters                                                        -//
//---------------------------------------------------------------------//
bool CHttpGetContext::isComplete() const
{
	return m_parseState == ParseState::Complete;
}

CHttpGetContext::ParseState CHttpGetContext::getParseState() const
{
	return m_parseState;
}

std::string CHttpGetContext::getHeaders() const
{
	return m_headers;
}

std::string &CHttpGetContext::getParseBuffer()
{
	return m_parseBuffer;
}

const std::string &CHttpGetContext::getParseBuffer() const
{
	return m_parseBuffer;
}

//---------------------------------------------------------------------//
//- Operations                                                        -//
//---------------------------------------------------------------------//
void CHttpGetContext::start(CTcpConnection *tcpConnection)
{
	m_tcpConnection = tcpConnection;
	m_parseState    = ParseState::Headers;
	m_headers.clear();
	m_parseBuffer.clear();

	// clang-format off
	m_timeoutTimer.async_wait([this](auto errorCode)
	{
		if (!errorCode && m_tcpConnection!=nullptr)
		{
			m_tcpConnection->stopRead();
		}
	});
	// clang-format on
}

void CHttpGetContext::append(std::span<const uint8_t> data)
{
	m_parseBuffer.append(reinterpret_cast<const char *>(data.data()), data.size());
	if (m_parseState == ParseState::Headers)
	{
		auto headerEnd = m_parseBuffer.find("\r\n\r\n");
		if (headerEnd != std::string::npos)
		{
			m_parseState  = ParseState::Body;
			m_headers     = m_parseBuffer.substr(0, headerEnd);
			m_parseBuffer = m_parseBuffer.substr(headerEnd + 4);
		}
	}
}

void CHttpGetContext::complete()
{
	cancel();
	m_parseState = ParseState::Complete;
}

void CHttpGetContext::cancel()
{
	m_timeoutTimer.cancel();
}
