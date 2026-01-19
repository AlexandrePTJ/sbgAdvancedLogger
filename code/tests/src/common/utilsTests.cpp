// Catch2 headers
#include <catch2/catch_test_macros.hpp>

// Project headers
#include "common/utils.h"

using namespace sbg::advlogger;

TEST_CASE("Base64 encoding - Empty string", "[base64]")
{
	REQUIRE(encodeBase64("").empty());
}

TEST_CASE("Base64 encoding - Single character", "[base64]")
{
	REQUIRE(encodeBase64("a") == "YQ==");
	REQUIRE(encodeBase64("A") == "QQ==");
	REQUIRE(encodeBase64("0") == "MA==");
}

TEST_CASE("Base64 encoding - Two characters", "[base64]")
{
	REQUIRE(encodeBase64("ab") == "YWI=");
	REQUIRE(encodeBase64("AB") == "QUI=");
}

TEST_CASE("Base64 encoding - Three characters (no padding)", "[base64]")
{
	REQUIRE(encodeBase64("abc") == "YWJj");
	REQUIRE(encodeBase64("ABC") == "QUJD");
}

TEST_CASE("Base64 encoding - Standard test vectors", "[base64]")
{
	// RFC 4648 test vectors
	REQUIRE(encodeBase64("f") == "Zg==");
	REQUIRE(encodeBase64("fo") == "Zm8=");
	REQUIRE(encodeBase64("foo") == "Zm9v");
	REQUIRE(encodeBase64("foob") == "Zm9vYg==");
	REQUIRE(encodeBase64("fooba") == "Zm9vYmE=");
	REQUIRE(encodeBase64("foobar") == "Zm9vYmFy");
}

TEST_CASE("Base64 encoding - Binary data", "[base64]")
{
	std::string binary;
	for (int i = 0; i < 256; i++)
	{
		binary += static_cast<char>(i);
	}

	std::string encoded = encodeBase64(binary);

	REQUIRE(encoded.size() == ((256 + 2) / 3) * 4);
	REQUIRE(encoded.size() % 4 == 0);

	for (char c : encoded)
	{
		REQUIRE(((c >= 'A' && c <= 'Z') ||
		         (c >= 'a' && c <= 'z') ||
		         (c >= '0' && c <= '9') ||
		         c == '+' || c == '/' || c == '='));
	}
}

TEST_CASE("Base64 encoding - NTRIP credentials example", "[base64]")
{
	REQUIRE(encodeBase64("user:pass") == "dXNlcjpwYXNz");
	REQUIRE(encodeBase64("admin:admin123") == "YWRtaW46YWRtaW4xMjM=");
}

TEST_CASE("Base64 encoding - Special characters", "[base64]")
{
	REQUIRE(encodeBase64("Hello World!") == "SGVsbG8gV29ybGQh");
	REQUIRE(encodeBase64("test@example.com") == "dGVzdEBleGFtcGxlLmNvbQ==");
	REQUIRE(encodeBase64("line1\nline2") == "bGluZTEKbGluZTI=");
	REQUIRE(encodeBase64("tab\there") == "dGFiCWhlcmU=");
}

TEST_CASE("Base64 encoding - UTF-8 characters", "[base64]")
{
	REQUIRE(encodeBase64("café") == "Y2Fmw6k=");
	REQUIRE(encodeBase64("日本語") == "5pel5pys6Kqe");
}

TEST_CASE("Base64 encoding - Long string", "[base64]")
{
	std::string long_input(1000, 'A');
	std::string encoded = encodeBase64(long_input);

	REQUIRE(encoded.size() == ((1000 + 2) / 3) * 4);
	REQUIRE(encoded.size() % 4 == 0);
}

TEST_CASE("Base64 encoding - All padding scenarios", "[base64]")
{
	REQUIRE(encodeBase64("123") == "MTIz");
	REQUIRE(encodeBase64("123456") == "MTIzNDU2");

	REQUIRE(encodeBase64("1") == "MQ==");
	REQUIRE(encodeBase64("1234") == "MTIzNA==");

	REQUIRE(encodeBase64("12") == "MTI=");
	REQUIRE(encodeBase64("12345") == "MTIzNDU=");
}

TEST_CASE("Base64 encoding - Null bytes", "[base64]")
{
	std::string with_nulls;
	with_nulls += '\0';
	with_nulls += "test";
	with_nulls += '\0';

	std::string encoded = encodeBase64(with_nulls);
	REQUIRE(!encoded.empty());
	REQUIRE(encoded.size() % 4 == 0);
}

TEST_CASE("NMEA checksum - Empty string", "[nmea][checksum]")
{
	REQUIRE(computeXORCrc("") == "00");
}

TEST_CASE("NMEA checksum - Standard GPGGA sentence", "[nmea][checksum]")
{
	// Sentence without $ and * : GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,
	REQUIRE(computeXORCrc("GPGGA,123519,4807.038,N,01131.000,E,1,08,0.9,545.4,M,46.9,M,,") == "47");
}

TEST_CASE("NMEA checksum - Standard GPRMC sentence", "[nmea][checksum]")
{
	// GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W
	REQUIRE(computeXORCrc("GPRMC,123519,A,4807.038,N,01131.000,E,022.4,084.4,230394,003.1,W") == "6A");
}

TEST_CASE("NMEA checksum - Single character", "[nmea][checksum]")
{
	REQUIRE(computeXORCrc("A") == "41");
	REQUIRE(computeXORCrc("B") == "42");
	REQUIRE(computeXORCrc("0") == "30");
}

TEST_CASE("NMEA checksum - Two characters XOR", "[nmea][checksum]")
{
	// 'A' (0x41) XOR 'A' (0x41) = 0x00
	REQUIRE(computeXORCrc("AA") == "00");
	// 'A' (0x41) XOR 'B' (0x42) = 0x03
	REQUIRE(computeXORCrc("AB") == "03");
}

TEST_CASE("NMEA checksum - Known test vectors", "[nmea][checksum]")
{
	// Common NMEA checksums
	REQUIRE(computeXORCrc("GPGLL,5107.0013414,N,11402.3279144,W,205412.00,A,A") == "73");
	REQUIRE(computeXORCrc("GNGGA,091408.00,4800.00000000,N,00200.00000000,E,1,01,0.0,0.000,M,0.000,M,,") == "49");
}
