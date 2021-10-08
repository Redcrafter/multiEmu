#pragma once

#include <array>
#include <fstream>
#include <string>

namespace Chip8 {

struct Chip8 {
	uint8_t V[16];
	std::array<uint8_t, 0x1000> memory;

	uint16_t I, PC;
	uint8_t delay_timer, sound_timer;

	uint8_t SP;
	uint16_t stack[0x100];

	uint8_t gfx[64 * 32];

  public:
	Chip8();

	void LoadRom(const std::string& path);

	void Reset();
	void Clock();
};

}
