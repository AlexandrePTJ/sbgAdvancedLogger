#include "analyzer/sbgEComAnalyzer.h"
#include "analyzer/sbgEComAnalyzer_p.h"

// 3rdparty headers
#include <interfaces/sbgInterface.h>
#include <spdlog/spdlog.h>

// Project headers
#include <common/nmea.h>

using namespace sbg::advlogger;

//---------------------------------------------------------------------//
//- sbgECom static implementation                                     -//
//---------------------------------------------------------------------//
namespace
{
	SbgErrorCode readBufferInterfaceImpl(SbgInterface *pHandle, void *pBuffer, size_t *pReadBytes, size_t bytesToRead)
	{
		SbgErrorCode errorCode     = SBG_NO_ERROR;
		auto        *pStreamBuffer = static_cast<SbgStreamBuffer *>(pHandle->handle);

		size_t availableBytes    = sbgStreamBufferGetSize(pStreamBuffer);
		size_t bytesToReadActual = (bytesToRead < availableBytes) ? bytesToRead : availableBytes;

		if (bytesToReadActual > 0)
		{
			errorCode   = sbgStreamBufferReadBuffer(pStreamBuffer, pBuffer, bytesToReadActual);
			*pReadBytes = bytesToReadActual;
		}
		else
		{
			*pReadBytes = 0;
		}

		return errorCode;
	}

	SbgErrorCode onSbgEComReceived(SbgEComHandle * /*pHandle*/, SbgEComClass msgClass, SbgEComMsgId msg, const SbgEComLogUnion *pLogData, void *pUserArg)
	{
		if (auto *pAnalyzer = static_cast<CSbgEComAnalyzerPrivate *>(pUserArg))
		{
			return pAnalyzer->processMessage(msgClass, msg, pLogData);
		}
		return SBG_ERROR;
	}

	std::chrono::system_clock::time_point getTimePointFromUtc(const SbgEComLogUtc &utc)
	{
		return std::chrono::system_clock::time_point{
		    std::chrono::duration_cast<std::chrono::system_clock::duration>(
		        std::chrono::sys_days{std::chrono::year{utc.year} / utc.month / utc.day}.time_since_epoch() + std::chrono::hours{utc.hour} + std::chrono::minutes{utc.minute} + std::chrono::seconds{utc.second} + std::chrono::nanoseconds{utc.nanoSecond})};
	}

	ENmeaGgaFixType gnssPosTypeTypeToNmeaPosQuality(SbgEComGnssPosType type)
	{
		switch (type)
		{
		case SBG_ECOM_GNSS_POS_TYPE_SINGLE:
		case SBG_ECOM_GNSS_POS_TYPE_FIXED:
			return ENmeaGgaFixType::Single;

		case SBG_ECOM_GNSS_POS_TYPE_OMNISTAR:
		case SBG_ECOM_GNSS_POS_TYPE_PSRDIFF:
		case SBG_ECOM_GNSS_POS_TYPE_SBAS:
			return ENmeaGgaFixType::DGPS;

		case SBG_ECOM_GNSS_POS_TYPE_RTK_FLOAT:
		case SBG_ECOM_GNSS_POS_TYPE_PPP_FLOAT:
			return ENmeaGgaFixType::RTKFloat;

		case SBG_ECOM_GNSS_POS_TYPE_RTK_INT:
		case SBG_ECOM_GNSS_POS_TYPE_PPP_INT:
			return ENmeaGgaFixType::RTKFixed;

		case SBG_ECOM_GNSS_POS_TYPE_NO_SOLUTION:
		case SBG_ECOM_GNSS_POS_TYPE_UNKNOWN:
		default:
			return ENmeaGgaFixType::Invalid;
		}
	}

	double computeHDOP(double latitudeStdDev, double longitudeStdDev)
	{
		return std::sqrt((latitudeStdDev * latitudeStdDev) + (longitudeStdDev * longitudeStdDev));
	}

	SNmeaGgaMessage createGGAFromSbgEComGNSS(const SbgEComLogGnssPos &gnssPos, const SbgEComLogUtc &utc)
	{
		return SNmeaGgaMessage{
		    .timestamp             = getTimePointFromUtc(utc),
		    .latitude              = gnssPos.latitude,
		    .longitude             = gnssPos.longitude,
		    .gpsQuality            = gnssPosTypeTypeToNmeaPosQuality(sbgEComLogGnssPosGetType(&gnssPos)),
		    .numSatellites         = gnssPos.numSvUsed,
		    .hdop                  = computeHDOP(gnssPos.latitudeAccuracy, gnssPos.longitudeAccuracy),
		    .orthometricHeight     = gnssPos.undulation + gnssPos.altitude,
		    .ageOfDifferentialData = gnssPos.differentialAge * 0.01,
		    .diffRefStationId      = gnssPos.baseStationId};
	}

	SNmeaGgaMessage createGGAFromSbgEComEkf(const SbgEComLogEkfNav &ekfNav, const SbgEComLogUtc &utc, const std::optional<SbgEComLogGnssPos> &gnssPos)
	{
		SNmeaGgaMessage message;
		message.timestamp         = getTimePointFromUtc(utc);
		message.latitude          = ekfNav.position[0];
		message.longitude         = ekfNav.position[1];
		message.hdop              = computeHDOP(ekfNav.positionStdDev[0], ekfNav.positionStdDev[1]);
		message.orthometricHeight = ekfNav.position[2];

		if (gnssPos.has_value())
		{
			message.gpsQuality            = gnssPosTypeTypeToNmeaPosQuality(sbgEComLogGnssPosGetType(&gnssPos.value()));
			message.numSatellites         = gnssPos->numSvUsed;
			message.ageOfDifferentialData = gnssPos->differentialAge * 0.01;
			message.diffRefStationId      = gnssPos->baseStationId;
		}

		return message;
	}

}// namespace

//---------------------------------------------------------------------//
//- Private Constructor / Destructor                                  -//
//---------------------------------------------------------------------//
CSbgEComAnalyzerPrivate::CSbgEComAnalyzerPrivate(const CSbgEComAnalyzer *pParent):
m_pParent(pParent),
m_buffer(),
m_interface(),
m_parserHandle()
{
}

CSbgEComAnalyzerPrivate::~CSbgEComAnalyzerPrivate()
{
	sbgInterfaceDestroy(&m_interface);
}

//---------------------------------------------------------------------//
//- Private Operations                                                -//
//---------------------------------------------------------------------//
void CSbgEComAnalyzerPrivate::initialize()
{
	sbgInterfaceZeroInit(&m_interface);
	m_interface.handle     = &m_buffer;
	m_interface.pReadFunc  = readBufferInterfaceImpl;
	m_interface.pWriteFunc = nullptr;
	m_interface.pFlushFunc = nullptr;
	m_interface.type       = SBG_IF_TYPE_UNKNOW;

	sbgEComInit(&m_parserHandle, &m_interface);
	sbgEComSetReceiveLogCallback(&m_parserHandle, onSbgEComReceived, this);
}

void CSbgEComAnalyzerPrivate::process(std::span<const uint8_t> data)
{
	sbgStreamBufferInitForRead(&m_buffer, data.data(), data.size());
	auto errorCode = sbgEComHandle(&m_parserHandle);

	if (errorCode != SBG_NO_ERROR && errorCode != SBG_NOT_READY)
	{
		spdlog::warn("sbgECom parsing error: {}", sbgErrorCodeToString(errorCode));
	}
}

SbgErrorCode CSbgEComAnalyzerPrivate::processMessage(SbgEComClass msgClass, SbgEComMsgId msg, const SbgEComLogUnion *pLogData)
{
	// Try to find position
	if (msgClass == SBG_ECOM_CLASS_LOG_ECOM_0)
	{
		switch (msg)
		{
		case SBG_ECOM_LOG_UTC_TIME:
			processTimeReference(pLogData->utcData);
			break;

		case SBG_ECOM_LOG_EKF_NAV:
			processEkvNav(pLogData->ekfNavData);
			break;

		case SBG_ECOM_LOG_GPS1_POS:
			processGnssPos(pLogData->gpsPosData, 1);
			break;

		case SBG_ECOM_LOG_GPS2_POS:
			processGnssPos(pLogData->gpsPosData, 2);
			break;

		default:
			break;
		}
	}

	return SBG_NO_ERROR;
}

void CSbgEComAnalyzerPrivate::processTimeReference(const SbgEComLogUtc &utcTimeReference)
{
	if (sbgEComLogUtcGetUtcStatus(&utcTimeReference) != SBG_ECOM_UTC_STATUS_INVALID)
	{
		m_lastUTCTimeReference = utcTimeReference;
	}
}

void CSbgEComAnalyzerPrivate::processEkvNav(const SbgEComLogEkfNav &ekfNav) const
{
	if (m_lastUTCTimeReference.has_value())
	{
		const auto &gnssPos = m_lastGnssPos[0].has_value() ? m_lastGnssPos[0] : m_lastGnssPos[1];

		if (m_pParent->m_positionCb)
		{
			auto position = createGGAFromSbgEComEkf(ekfNav, m_lastUTCTimeReference.value(), gnssPos);
			m_pParent->m_positionCb(position);
		}
	}
}

void CSbgEComAnalyzerPrivate::processGnssPos(const SbgEComLogGnssPos &gnssPos, uint8_t index)
{
	if (index != 1 && index != 2)
	{
		throw std::invalid_argument("Invalid index");
	}

	m_lastGnssPos[index - 1] = gnssPos;

	if (m_lastUTCTimeReference.has_value())
	{
		if (m_pParent->m_positionCb)
		{
			auto position = createGGAFromSbgEComGNSS(gnssPos, m_lastUTCTimeReference.value());
			m_pParent->m_positionCb(position);
		}
	}
}

//---------------------------------------------------------------------//
//- Public implementation                                             -//
//---------------------------------------------------------------------//

//---------------------------------------------------------------------//
//- Constructor / Destructor                                          -//
//---------------------------------------------------------------------//
CSbgEComAnalyzer::CSbgEComAnalyzer():
m_pImpl(std::make_unique<CSbgEComAnalyzerPrivate>(this))
{
	m_pImpl->initialize();
}

CSbgEComAnalyzer::~CSbgEComAnalyzer() = default;

//---------------------------------------------------------------------//
//- Parameters                                                        -//
//---------------------------------------------------------------------//
std::string CSbgEComAnalyzer::getId() const
{
	return "sbgecom";
}

//---------------------------------------------------------------------//
//- Operations                                                        -//
//---------------------------------------------------------------------//
void CSbgEComAnalyzer::process(std::span<const uint8_t> data)
{
	m_pImpl->process(data);
}
