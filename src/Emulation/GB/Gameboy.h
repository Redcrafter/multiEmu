#pragma once
#include <cstdint>
#include <memory>

#include "APU.h"
#include "LR35902.h"
#include "Mappers/MBC.h"
#include "PPU.h"

namespace Gameboy {

enum class Interrupt {
	VBlank = 0,
	LCDStat = 1,
	Timer = 2,
	Serial = 3,
	Joypad = 4
};

class Gameboy {
	friend class GameboyColorCore;
	friend class LR35902;

  private:
	LR35902 cpu;
	PPU ppu;
	APU apu;

	uint8_t ramBank;
	uint8_t ram[8][0x1000];

	uint8_t hram[127];

	uint8_t InterruptEnable;
	uint8_t InterruptFlag;

	bool JoyPadSelect;
	uint8_t SB;
	uint8_t SC;
	uint16_t DIV;
	uint8_t TIMA;
	uint8_t TMA;
	uint8_t TAC;

	bool inBios;

  public:
	bool gbc = false;
	std::unique_ptr<MBC> mbc;

	Gameboy(RenderImage& texture) : cpu(*this), ppu(*this, texture) {
		Reset();
	}

	void Reset();

	void Clock();

	void Interrupt(Interrupt interrupt) {
		InterruptFlag |= 1 << (int)interrupt;
	}

	uint8_t CpuRead(uint16_t addr) const;
	void CpuWrite(uint16_t addr, uint8_t val);
};

}
