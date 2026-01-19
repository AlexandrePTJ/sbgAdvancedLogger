#include "tcpConnection.h"

#include <spdlog/spdlog.h>

using namespace sbg::advlogger;

//---------------------------------------------------------------------//
//- Constructor / Destructor                                          -//
//---------------------------------------------------------------------//
CTcpConnection::CTcpConnection(asio::io_context &ioc, const STcpConfig &tcpConfig, int32_t readBufferSize, int32_t sendBufferSize, int32_t receiveBufferSize):
m_ioc(ioc),
m_tcpConfig(tcpConfig),
m_sendBufferSize(sendBufferSize),
m_receiveBufferSize(receiveBufferSize),
m_socket(ioc),
m_readBuffer(readBufferSize)
{
}

CTcpConnection::~CTcpConnection()
{
	close();
}

//---------------------------------------------------------------------//
//- Operations                                                        -//
//---------------------------------------------------------------------//
void CTcpConnection::asyncConnect(const ConnectCallback &callback)
{
	if (m_isConnected)
	{
		if (callback)
		{
			callback({});
		}
		return;
	}

	// Resolve hostname
	auto resolver = std::make_shared<asio::ip::tcp::resolver>(m_ioc);

	resolver->async_resolve(
	    m_tcpConfig.host,
	    std::to_string(m_tcpConfig.port),
	    [this, callback, resolver](auto ec, auto endpoints)
	    {
		    if (ec)
		    {
			    spdlog::error("TCP resolve failed for {} - {}", m_tcpConfig.toString(), ec.message());
			    if (callback)
			    {
				    callback(ec);
			    }
			    return;
		    }

		    // Connect to resolved endpoint
		    asio::async_connect(
		        m_socket,
		        endpoints,
		        [this, callback](auto ec, auto /*endpoint*/)
		        {
			        if (ec)
			        {
				        spdlog::error("TCP connection failed to {} - {}", m_tcpConfig.toString(), ec.message());
				        if (callback)
				        {
					        callback(ec);
				        }
				        return;
			        }

			        // Set socket options
			        m_socket.set_option(asio::socket_base::receive_buffer_size(m_receiveBufferSize));
			        m_socket.set_option(asio::socket_base::send_buffer_size(m_sendBufferSize));

			        m_isConnected = true;
			        spdlog::info("TCP connected to {}", m_tcpConfig.toString());

			        if (callback)
			        {
				        callback({});
			        }
		        });
	    });
}

void CTcpConnection::startRead(DataCallback dataCb, ErrorCallback errorCb)
{
	m_dataCb  = std::move(dataCb);
	m_errorCb = std::move(errorCb);

	if (!m_isConnected)
	{
		spdlog::warn("Cannot start read: not connected");
		return;
	}

	m_isReading = true;
	doRead();
}

void CTcpConnection::stopRead()
{
	m_isReading = false;
}

void CTcpConnection::asyncWrite(std::span<const uint8_t> data, const WriteCallback &writeCb)
{
	if (!m_isConnected)
	{
		if (writeCb)
		{
			writeCb(std::make_error_code(std::errc::not_connected));
		}
		return;
	}

	// Create a shared buffer to keep data alive during async operation
	auto buffer = std::make_shared<std::vector<uint8_t>>(data.begin(), data.end());

	asio::async_write(
	    m_socket,
	    asio::buffer(*buffer),
	    [this, writeCb, buffer](auto ec, std::size_t bytes_transferred)
	    {
		    if (ec)
		    {
			    spdlog::warn("TCP write error ({}): {}", m_tcpConfig.toString(), ec.message());
			    if (writeCb)
			    {
				    writeCb(ec);
			    }
		    }
		    else
		    {
			    spdlog::trace("TCP write ({}): {} bytes", m_tcpConfig.toString(), bytes_transferred);
			    if (writeCb)
			    {
				    writeCb({});
			    }
		    }
	    });
}

void CTcpConnection::asyncWrite(std::string_view data, const WriteCallback &writeCb)
{
	asyncWrite(std::span(reinterpret_cast<const uint8_t *>(data.data()), data.size()), writeCb);
}

std::error_code CTcpConnection::write(std::span<const uint8_t> data)
{
	if (!m_isConnected)
	{
		return std::make_error_code(std::errc::not_connected);
	}

	std::error_code ec;

	asio::write(m_socket, asio::buffer(data.data(), data.size()), ec);

	if (ec)
	{
		spdlog::warn("TCP sync write error ({}): {}", m_tcpConfig.toString(), ec.message());
		return ec;
	}

	return {};
}

std::error_code CTcpConnection::write(std::string_view data)
{
	return write(std::span(reinterpret_cast<const uint8_t *>(data.data()), data.size()));
}

void CTcpConnection::close()
{
	if (m_isConnected || m_socket.is_open())
	{
		std::error_code ec;

		m_isReading = false;

		// Shutdown both send and receive
		m_socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
		if (ec && ec != asio::error::not_connected)
		{
			spdlog::debug("TCP shutdown error ({}): {}", m_tcpConfig.toString(), ec.message());
		}

		m_socket.close(ec);
		if (ec)
		{
			spdlog::debug("TCP close error ({}): {}", m_tcpConfig.toString(), ec.message());
		}

		m_isConnected = false;
		spdlog::info("TCP connection closed ({})", m_tcpConfig.toString());
	}
}

bool CTcpConnection::isConnected() const
{
	return m_isConnected;
}

asio::ip::tcp::socket &CTcpConnection::socket()
{
	return m_socket;
}

//---------------------------------------------------------------------//
//- Internal operations                                               -//
//---------------------------------------------------------------------//
void CTcpConnection::doRead()
{
	if (!m_isReading)
	{
		return;
	}

	m_socket.async_read_some(
	    asio::buffer(m_readBuffer),
	    [this](auto ec, std::size_t bytes_transferred)
	    {
		    if (ec)
		    {
			    handleReadError(ec);
			    return;
		    }

		    // Forward data to callback
		    if (m_dataCb && bytes_transferred > 0)
		    {
			    m_dataCb(std::span<const uint8_t>(m_readBuffer.data(), bytes_transferred));
		    }

		    // Continue reading
		    doRead();
	    });
}

void CTcpConnection::handleReadError(const std::error_code &ec)
{
	if (ec == asio::error::eof)
	{
		spdlog::info("TCP connection closed by peer ({})", m_tcpConfig.toString());
	}
	else if (ec == asio::error::operation_aborted)
	{
		spdlog::debug("TCP read operation aborted ({})", m_tcpConfig.toString());
	}
	else
	{
		spdlog::error("TCP read error ({}): {}", m_tcpConfig.toString(), ec.message());
	}

	m_isConnected = false;
	m_isReading   = false;

	if (m_errorCb)
	{
		m_errorCb(ec);
	}
}
