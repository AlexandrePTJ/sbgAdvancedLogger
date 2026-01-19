#include "nmea.h"

// 3rdparty headers
#include <fmt/format.h>

// Local headers
#include "utils.h"

using namespace sbg::advlogger;

//---------------------------------------------------------------------//
//- Internal helpers                                                  -//
//---------------------------------------------------------------------//
namespace
{
	std::string formatLatitude(double latitude)
	{
		char hemisphere = latitude >= 0 ? 'N' : 'S';
		latitude        = std::abs(latitude);

		int    degrees = static_cast<int>(latitude);
		double minutes = (latitude - degrees) * 60.0;

		return fmt::format("{:02d}{:09.6f},{}", degrees, minutes, hemisphere);
	}

	std::string formatLongitude(double longitude)
	{
		char hemisphere = longitude >= 0 ? 'E' : 'W';
		longitude       = std::abs(longitude);

		int    degrees = static_cast<int>(longitude);
		double minutes = (longitude - degrees) * 60.0;

		return fmt::format("{:03d}{:09.6f},{}", degrees, minutes, hemisphere);
	}

	std::string formatTime(const std::chrono::system_clock::time_point &time)
	{
		auto days = std::chrono::floor<std::chrono::days>(time);
		auto tod  = std::chrono::hh_mm_ss{time - days};

		auto centiseconds = std::chrono::duration_cast<std::chrono::duration<int, std::centi>>(
		                        tod.subseconds())
		                        .count();

		return fmt::format("{:02d}{:02d}{:02d}.{:02d}",
		                   tod.hours().count(),
		                   tod.minutes().count(),
		                   tod.seconds().count(),
		                   centiseconds);
	}

}// namespace

//---------------------------------------------------------------------//
//- CNmeaFactory Operations                                           -//
//---------------------------------------------------------------------//
SNmeaGgaMessage CNmeaFactory::createGGAFromSimplePosition(double latitude, double longitude)
{
	return SNmeaGgaMessage{
	    .timestamp             = std::chrono::system_clock::now(),
	    .latitude              = latitude,
	    .longitude             = longitude,
	    .gpsQuality            = ENmeaGgaFixType::Single,
	    .numSatellites         = 12,
	    .hdop                  = 0.1,
	    .orthometricHeight     = 0,
	    .ageOfDifferentialData = 0,
	    .diffRefStationId      = 0};
}

//---------------------------------------------------------------------//
//- CNmeaEncoder Operations                                           -//
//---------------------------------------------------------------------//

std::string CNmeaEncoder::encode(const SNmeaGgaMessage &ggaMessage)
{
	auto sentence = fmt::format("GPGGA,{},{},{},{},{},{:.2f},{:.1f},M,,M,{:.1f},{:04d}",
	                            formatTime(ggaMessage.timestamp),
	                            formatLatitude(ggaMessage.latitude),
	                            formatLongitude(ggaMessage.longitude),
	                            static_cast<uint8_t>(ggaMessage.gpsQuality),
	                            ggaMessage.numSatellites,
	                            ggaMessage.hdop,
	                            ggaMessage.orthometricHeight,
	                            ggaMessage.ageOfDifferentialData,
	                            ggaMessage.diffRefStationId);

	std::string checksum = computeXORCrc(sentence);

	return std::format("${}*{}", sentence, checksum);
}
