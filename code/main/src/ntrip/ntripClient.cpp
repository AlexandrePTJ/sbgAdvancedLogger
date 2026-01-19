#include "ntripClient.h"

// #include "rtcm_forwarder.h"

// 3rdparty headers
#include <spdlog/spdlog.h>

// Project headers
#include <common/httpGetContext.h>
#include <common/utils.h>

using namespace sbg::advlogger;

//---------------------------------------------------------------------//
//- Static helpers                                                    -//
//---------------------------------------------------------------------//
namespace
{
	std::string buildNtripHttpRequest(const std::string &host, const std::string &mountPoint = "", const std::string &username = "", const std::string &password = "", const std::string &gga = "")
	{
		auto request = fmt::format(
		    "GET /{} HTTP/1.1\r\n"
		    "Host: {}\r\n"
		    "User-Agent: NTRIP sbgAdvancedLogger/1.0\r\n"
		    "Ntrip-Version: Ntrip/2.0\r\n"
		    "Connection: close\r\n",
		    mountPoint,
		    host);

		if (!username.empty() && !password.empty())
		{
			request += fmt::format("Authorization: Basic {}\r\n", encodeBase64(username + ":" + password));
		}

		if (!gga.empty())
		{
			request += fmt::format("Ntrip-GGA: {}\r\n", gga);
		}

		request += "\r\n";

		return request;
	}
}// namespace

//---------------------------------------------------------------------//
//- Constructor / Destructor                                          -//
//---------------------------------------------------------------------//
CNtripClient::CNtripClient(asio::io_context &ioc, const SNtripConfig &ntripConfig):
m_ioc(ioc),
m_ntripConfig(ntripConfig),
m_tcpConnection(ioc, STcpConfig{ntripConfig.host, ntripConfig.port}, 4096, 8192, 8192),
m_ggaTimer(ioc)
{
	// m_ggaFrame = "$GPGGA,150003.00,4854.597058,N,00210.042416,E,2,33,0.41,65.7,M,,M,1.4,0123*51";
}

CNtripClient::~CNtripClient()
{
	disconnect();
}

//---------------------------------------------------------------------//
//- Parameters                                                        -//
//---------------------------------------------------------------------//
bool CNtripClient::isConnected() const
{
	return m_tcpConnection.isConnected();
}

void CNtripClient::setGgaFrame(const std::string &gga)
{
	m_ggaFrame = gga;
}

//---------------------------------------------------------------------//
//- Operations                                                        -//
//---------------------------------------------------------------------//
std::error_code CNtripClient::connect()
{
	std::error_code result;
	bool            connectFinished = false;

	// clang-format off
	m_tcpConnection.asyncConnect([&](std::error_code errorCode)
	{
		result = errorCode;
		connectFinished = true;
	});
	// clang-format on

	// Run io_context until the connection completes
	while (!connectFinished)
	{
		m_ioc.run_one();
	}

	return result;
}

void CNtripClient::disconnect()
{
	m_ggaTimer.cancel();
	m_tcpConnection.close();
}

void CNtripClient::requestSourceTable(SourceTableCallback sourceTableCb, ErrorCallback errorCb)
{
	if (!m_tcpConnection.isConnected())
	{
		if (errorCb)
		{
			errorCb(std::make_error_code(std::errc::not_connected));
		}
		return;
	}

	auto request = buildNtripHttpRequest(m_ntripConfig.host);

	// clang-format off
	m_tcpConnection.asyncWrite(request, [this, sourceTableCb, errorCb](std::error_code errorCode)
	{
		if (errorCode)
		{
			if (errorCb)
			{
				errorCb(errorCode);
			}
			return;
		}

		auto ctx = std::make_shared<CHttpGetContext>(m_ioc);
		ctx->start(&m_tcpConnection);

		m_tcpConnection.startRead([this, ctx, sourceTableCb](std::span<const uint8_t> data)
		{
			ctx->append(data);

			if (ctx->getParseState() == CHttpGetContext::ParseState::Body)
			{
				if (ctx->getParseBuffer().find("ENDSOURCETABLE") != std::string::npos)
				{
					ctx->complete();
					if (sourceTableCb)
					{
						sourceTableCb(ctx->getParseBuffer());
					}
					m_tcpConnection.stopRead();
				}
			}
		},
		[ctx, errorCb](std::error_code ec)
		{
			ctx->cancel();
			if (errorCb)
			{
				errorCb(ec);
			}
		});
	});
	// clang-format on
}

void CNtripClient::startListenMountPoint(RtcmCallback rtcmCb, ErrorCallback errorCb)
{
	if (!m_tcpConnection.isConnected())
	{
		if (errorCb)
		{
			errorCb(std::make_error_code(std::errc::not_connected));
		}
		return;
	}

	auto request = buildNtripHttpRequest(m_ntripConfig.host, m_ntripConfig.mountpoint, m_ntripConfig.username, m_ntripConfig.password, m_ggaFrame);

	// clang-format off
	m_tcpConnection.asyncWrite(request, [this, rtcmCb, errorCb](std::error_code errorCode)
	{
		if (errorCode)
		{
			if (errorCb)
			{
				errorCb(errorCode);
			}
			return;
		}

		auto ctx = std::make_shared<CHttpGetContext>(m_ioc);
		ctx->start(&m_tcpConnection);

		m_tcpConnection.startRead([rtcmCb, ctx](std::span<const uint8_t> data)
		{
			if (ctx->getParseState() == CHttpGetContext::ParseState::Headers)
			{
				ctx->append(data);
			}
			else if (ctx->getParseState() == CHttpGetContext::ParseState::Body)
			{
				if (rtcmCb)
				{
					rtcmCb(data);
				}
			}
		},
		[ctx, errorCb](std::error_code ec)
		{
			ctx->cancel();
			if (errorCb)
			{
				errorCb(ec);
			}
		});

		scheduleGgaSend();
	});
	// clang-format on
}

//---------------------------------------------------------------------//
//- Internal operations                                               -//
//---------------------------------------------------------------------//
void CNtripClient::scheduleGgaSend()
{
	m_ggaTimer.expires_after(m_ntripConfig.ggaSendingInterval);

	// clang-format off
	m_ggaTimer.async_wait([this](std::error_code ec)
	{
		if (!ec && isConnected() && !m_ggaFrame.empty())
		{
			sendGgaFrame();
			scheduleGgaSend();
		}
	});
	// clang-format on
}

void CNtripClient::sendGgaFrame()
{
	if (!m_tcpConnection.isConnected())
	{
		spdlog::warn("Cannot send GGA frame: not connected");
		return;
	}

	// clang-format off
	m_tcpConnection.asyncWrite(m_ggaFrame, [](std::error_code ec)
	{
		if (ec)
		{
			spdlog::warn("Failed to send GGA frame: {}", ec.message());
		}
	});
	// clang-format on
}
