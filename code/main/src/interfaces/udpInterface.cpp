#include "udpInterface.h"
#include <spdlog/spdlog.h>

using namespace sbg::advlogger;

CUdpInterface::CUdpInterface(asio::io_context &ioc, const SUdpConfig &config):
CAbstractInterface(ioc),
m_udpConfig(config),
m_socket(ioc),
m_readBuffer()
{
}

CUdpInterface::~CUdpInterface()
{
	stop();
}

std::error_code CUdpInterface::start(DataCallback data_cb, ErrorCallback error_cb)
{
	m_dataCb  = std::move(data_cb);
	m_errorCb = std::move(error_cb);

	try
	{
		std::error_code ec;

		// Open UDP socket
		m_socket.open(asio::ip::udp::v4(), ec);
		if (ec)
		{
			spdlog::error("UDP socket open failed: {}", ec.message());
			return ec;
		}

		// Set socket options
		m_socket.set_option(asio::socket_base::reuse_address(true), ec);
		m_socket.set_option(asio::socket_base::receive_buffer_size(65536), ec);

		// Bind to port
		asio::ip::udp::endpoint listenEndpoint(asio::ip::udp::v4(), m_udpConfig.portIn);
		m_socket.bind(listenEndpoint, ec);
		if (ec)
		{
			spdlog::error("UDP bind failed on port {}: {}", m_udpConfig.portIn, ec.message());
			return ec;
		}
		spdlog::info("UDP bind on port :{}", m_udpConfig.portIn);

		// If we have a specific host configured, resolve it for sending
		if (!m_udpConfig.host.empty())
		{
			asio::ip::udp::resolver resolver(m_ioc);
			auto                    endpoints = resolver.resolve(
                asio::ip::udp::v4(),
                m_udpConfig.host,
                std::to_string(m_udpConfig.portOut),
                ec);

			if (!ec && endpoints.begin() != endpoints.end())
			{
				m_remoteOutEndpoint = *endpoints.begin();
				spdlog::info("UDP remote endpoint set to {}:{}",
				             m_udpConfig.host,
				             m_udpConfig.portOut);
			}
		}

		m_isActive = true;
		doRead();

		return {};
	}
	catch (const std::exception &e)
	{
		spdlog::error("UDP source exception: {}", e.what());
		return std::make_error_code(std::errc::connection_refused);
	}
}

void CUdpInterface::doRead()
{
	m_socket.async_receive_from(
	    asio::buffer(m_readBuffer),
	    m_remoteInEndpoint,// Will be filled with sender's endpoint
	    [this](auto ec, std::size_t bytes_transferred)
	    {
		    if (ec)
		    {
			    if (ec == asio::error::operation_aborted)
			    {
				    spdlog::debug("UDP read operation aborted");
			    }
			    else
			    {
				    spdlog::error("UDP read error: {}", ec.message());
			    }

			    m_isActive = false;
			    if (m_errorCb)
			    {
				    m_errorCb(ec);
			    }
			    return;
		    }

		    // Log sender for debugging (can be removed in production)
		    spdlog::trace("UDP received {} bytes from {}:{}",
		                  bytes_transferred,
		                  m_remoteInEndpoint.address().to_string(),
		                  m_remoteInEndpoint.port());

		    // Forward data to callback
		    if (m_dataCb && bytes_transferred > 0)
		    {
			    m_dataCb(std::span<const uint8_t>(m_readBuffer.data(), bytes_transferred));
		    }

		    // Continue reading
		    doRead();
	    });
}

void CUdpInterface::stop()
{
	if (m_isActive)
	{
		std::error_code ec;
		m_socket.close(ec);
		if (ec)
		{
			spdlog::debug("UDP close error: {}", ec.message());
		}

		m_isActive = false;
		spdlog::info("UDP source stopped");
	}
}

void CUdpInterface::asyncWrite(std::span<const uint8_t> data, ErrorCallback errorCb)
{
	if (!m_isActive)
	{
		if (errorCb)
		{
			errorCb(std::make_error_code(std::errc::not_connected));
		}
		return;
	}

	// Check if we have a valid remote endpoint
	if (m_remoteOutEndpoint.port() == 0)
	{
		spdlog::warn("UDP write: no remote endpoint configured");
		if (errorCb)
		{
			errorCb(std::make_error_code(std::errc::destination_address_required));
		}
		return;
	}

	// Create a shared buffer to keep data alive during async operation
	auto buffer = std::make_shared<std::vector<uint8_t>>(data.begin(), data.end());

	m_socket.async_send_to(
	    asio::buffer(*buffer),
	    m_remoteOutEndpoint,
	    [errorCb, buffer](auto ec, std::size_t bytes_transferred)
	    {
		    if (ec)
		    {
			    spdlog::warn("UDP write error: {}", ec.message());
			    if (errorCb)
			    {
				    errorCb(ec);
			    }
		    }
		    else
		    {
			    spdlog::trace("UDP write: {} bytes", bytes_transferred);
			    if (errorCb)
			    {
				    errorCb({});
			    }
		    }
	    });
}
