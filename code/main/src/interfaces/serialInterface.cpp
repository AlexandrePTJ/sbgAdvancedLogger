#include "serialInterface.h"

#include <spdlog/spdlog.h>

using namespace sbg::advlogger;

//---------------------------------------------------------------------//
//- Static helpers                                                    -//
//---------------------------------------------------------------------//
static asio::serial_port::parity::type getParityFromString(const std::string &str)
{
	if (str == "none")
	{
		return asio::serial_port::parity::none;
	}
	if (str == "odd")
	{
		return asio::serial_port_base::parity::odd;
	}
	if (str == "even")
	{
		return asio::serial_port_base::parity::even;
	}
	throw std::runtime_error("Invalid parity");
}

static asio::serial_port::stop_bits::type getStopBitsFromString(const std::string &str)
{
	if (str == "one")
	{
		return asio::serial_port::stop_bits::one;
	}
	if (str == "two")
	{
		return asio::serial_port::stop_bits::two;
	}
	throw std::runtime_error("Invalid stop bits");
}

static asio::serial_port::flow_control::type getFlowControlFromString(const std::string &str)
{
	if (str == "none")
	{
		return asio::serial_port::flow_control::none;
	}
	if (str == "hardware")
	{
		return asio::serial_port::flow_control::hardware;
	}
	if (str == "software")
	{
		return asio::serial_port::flow_control::software;
	}
	throw std::runtime_error("Invalid flow control");
}

//---------------------------------------------------------------------//
//- Constructor / Destructor                                          -//
//---------------------------------------------------------------------//
CSerialInterface::CSerialInterface(asio::io_context &ioc, const SSerialConfig &config):
CAbstractInterface(ioc),
m_serialPort(ioc),
m_serialConfig(config),
m_readBuffer()
{
}

CSerialInterface::~CSerialInterface()
{
	CSerialInterface::stop();
}

//---------------------------------------------------------------------//
//- Operations                                                        -//
//---------------------------------------------------------------------//
std::error_code CSerialInterface::start(DataCallback dataCb, ErrorCallback errorCb)
{
	m_dataCb  = std::move(dataCb);
	m_errorCb = std::move(errorCb);

	std::error_code ec;

	// Open serial port
	m_serialPort.open(m_serialConfig.port, ec);
	if (ec)
	{
		spdlog::error("Failed to open serial port {}: {}", m_serialConfig.port, ec.message());
		return std::make_error_code(std::errc::io_error);
	}

	// Configure serial port
	m_serialPort.set_option(asio::serial_port_base::baud_rate(m_serialConfig.baudRate));
	m_serialPort.set_option(asio::serial_port_base::character_size(m_serialConfig.dataBits));
	m_serialPort.set_option(asio::serial_port_base::parity(getParityFromString(m_serialConfig.parity)));
	m_serialPort.set_option(asio::serial_port_base::stop_bits(getStopBitsFromString(m_serialConfig.stopBits)));
	m_serialPort.set_option(asio::serial_port_base::flow_control(getFlowControlFromString(m_serialConfig.flowControl)));

	m_isActive = true;
	doRead();

	spdlog::info("Serial port {} opened at {} baud", m_serialConfig.port, m_serialConfig.baudRate);
	return {};
}

void CSerialInterface::stop()
{
	if (m_isActive)
	{
		std::error_code ec;
		m_serialPort.close(ec);
		m_isActive = false;
		spdlog::info("Serial port closed");
	}
}

void CSerialInterface::asyncWrite(std::span<const uint8_t> data, ErrorCallback errorCb)
{
	asio::async_write(
	    m_serialPort,
	    asio::buffer(data.data(), data.size()),
	    [errorCb](std::error_code ec, std::size_t)
	    {
		    if (ec)
		    {
			    errorCb(std::make_error_code(std::errc::io_error));
		    }
		    else
		    {
			    errorCb({});
		    }
	    });
}

//---------------------------------------------------------------------//
//- Internal operations                                               -//
//---------------------------------------------------------------------//
void CSerialInterface::doRead()
{
	m_serialPort.async_read_some(
	    asio::buffer(m_readBuffer),
	    [this](std::error_code ec, std::size_t bytesTransferred)
	    {
		    if (ec)
		    {
			    spdlog::error("Serial read error: {}", ec.message());
			    m_isActive = false;
			    if (m_errorCb)
			    {
				    m_errorCb(std::make_error_code(std::errc::io_error));
			    }
			    return;
		    }

		    if (m_dataCb)
		    {
			    m_dataCb(std::span<const uint8_t>(m_readBuffer.data(), bytesTransferred));
		    }

		    doRead();
	    });
}
