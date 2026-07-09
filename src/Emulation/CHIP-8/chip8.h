#pragma once
#include <array>
#include <string>

namespace Chip8 {

struct Chip8 {
	std::array<uint8_t, 16> V;
	std::array<uint8_t, 0x1000> memory;

	uint16_t I, PC;
	uint8_t delay_timer, sound_timer;

	uint8_t SP;
	std::array<uint16_t, 0x100> stack;
	std::array<uint8_t, 64 * 32> gfx;

  public:
	Chip8();

	void LoadRom(const std::string& path);

	void Reset();
	void Clock();
};

}
