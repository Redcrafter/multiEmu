#pragma once
#include <cstdint>

namespace math {

inline uint32_t interleave(uint16_t low, uint16_t high) {
	uint32_t x = low;
	uint32_t y = high;

	x = (x | (x << 8)) & 0x00FF00FF;
	x = (x | (x << 4)) & 0x0F0F0F0F;
	x = (x | (x << 2)) & 0x33333333;
	x = (x | (x << 1)) & 0x55555555;

	y = (y | (y << 8)) & 0x00FF00FF;
	y = (y | (y << 4)) & 0x0F0F0F0F;
	y = (y | (y << 2)) & 0x33333333;
	y = (y | (y << 1)) & 0x55555555;

	return x | (y << 1);
}

inline uint16_t interleave(uint8_t low, uint8_t high) {
	uint16_t x = low;
	uint16_t y = high;

	x = (x | (x << 4)) & 0x0F0F;
	x = (x | (x << 2)) & 0x3333;
	x = (x | (x << 1)) & 0x5555;

	y = (y | (y << 4)) & 0x0F0F;
	y = (y | (y << 2)) & 0x3333;
	y = (y | (y << 1)) & 0x5555;

	return x | (y << 1);
}

inline uint8_t reverse(uint8_t b) {
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	return b;
}

} // namespace math
