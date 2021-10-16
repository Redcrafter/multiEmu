#pragma once
#include "MBC.h"

namespace Gameboy {

class MBC2 final : public MBC {
  private:
	bool ramEnable = false;
	uint32_t romBank = 0x4000;

  public:
	MBC2(const std::vector<uint8_t>& rom, bool hasBattery) : MBC(rom, 512, hasBattery) {}
	~MBC2() override = default;

	uint8_t Read0(uint16_t addr) const override { return rom[addr]; }
	uint8_t Read4(uint16_t addr) const override { return rom[romBank | (addr & 0x3FFF)]; }
	uint8_t ReadA(uint16_t addr) const override { return ramEnable ? ram[addr & 0x1FF] | 0xF0 : 0xFF; }

	void Write0(uint16_t addr, uint8_t val) override {
		if(addr & 0x100) {
			romBank = (std::max(val & 0xF, 1) << 14) & romMask;
		} else {
			ramEnable = (val & 0xF) == 0xA;
		}
	};
	void Write4(uint16_t addr, uint8_t val) override {};
	void WriteA(uint16_t addr, uint8_t val) override { 
		if(ramEnable) ram[addr & 0x1FF] = val;
	};
};

}
