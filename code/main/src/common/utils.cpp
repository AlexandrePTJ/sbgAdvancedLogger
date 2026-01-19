#include "utils.h"

// STL headers
#include <algorithm>
#include <array>
#include <numeric>
#include <cstdint>

namespace sbg::advlogger
{
	static const auto gBase64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

	std::string encodeBase64(const std::string &input)
	{
		std::string result;
		result.reserve(((input.size() + 2) / 3) * 4);

		int val  = 0;
		int valb = -6;

		for (unsigned char c : input)
		{
			val = (val << 8) + c;
			valb += 8;
			while (valb >= 0)
			{
				result.push_back(gBase64Chars[(val >> valb) & 0x3F]);
				valb -= 6;
			}
		}

		if (valb > -6)
		{
			result.push_back(gBase64Chars[(val << (-valb)) & 0x3F]);
		}

		while (result.size() % 4)
		{
			result.push_back('=');
		}

		return result;
	}

	std::string computeXORCrc(const std::string &data)
	{
		const uint8_t checksum = std::accumulate(data.begin(), data.end(), uint8_t{0}, [](uint8_t acc, char c)
		                                         { return acc ^ static_cast<uint8_t>(c); });

		std::array<char, 3> buffer;
		std::snprintf(buffer.data(), buffer.size(), "%02X", checksum);
		return {buffer.data()};
	}
}// namespace sbg::advlogger