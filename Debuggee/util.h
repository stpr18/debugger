#pragma once
#include <cstdint>

namespace util
{
	inline uint8_t int32toq0(uint32_t x)
	{
		return x >> 24;
	}

	inline uint8_t int32toq1(uint32_t x)
	{
		return (x >> 16) & 0xff;
	}

	inline uint8_t int32toq2(uint32_t x)
	{
		return (x >> 8) & 0xff;
	}

	inline uint8_t int32toq3(uint32_t x)
	{
		return x & 0xff;
	}
}
