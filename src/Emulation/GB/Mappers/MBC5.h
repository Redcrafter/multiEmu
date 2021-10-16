#pragma once
#include "MBC.h"

namespace Gameboy {

class MBC5 final : public MBC {
  private:
	bool ramEnable = false;
	bool hasRumble;

	uint32_t romBank = 0x4000;
	uint32_t ramBank = 0;

  public:
	MBC5(const std::vector<uint8_t>& rom, uint32_t ramSize, bool hasBattery, bool hasRumble) : MBC(rom, ramSize, hasBattery), hasRumble(hasRumble) {}
	~MBC5() override = default;

	uint8_t Read0(uint16_t addr) const override { return rom[addr & 0x3FFF]; }
	uint8_t Read4(uint16_t addr) const override { return rom[romBank | (addr & 0x3FFF)]; };
	uint8_t ReadA(uint16_t addr) const override {
        return ramEnable ? ram[ramBank | (addr & 0x1FFF)] : 0xFF;
	};

	void Write0(uint16_t addr, uint8_t val) override {
		if(addr < 0x2000) {
			// 0000-1FFF - RAM Enable
			ramEnable = !ram.empty() && (val & 0xF) == 0xA;
		} else if(addr < 0x3000) {
			// 2000-2FFF - 8 least significant bits of ROM bank number
			romBank = ((romBank & 0x400000) | (0x4000 * val)) & romMask;
		} else {
            // 3000-3FFF - 9th bit of ROM bank number
			romBank = ((romBank & 0x3FFFFF) | ((val & 1) * 0x400000)) & romMask;
        }
	};
	void Write4(uint16_t addr, uint8_t val) override {
		if(addr < 0x6000) {
			// 4000-5FFF - RAM bank number
			ramBank = ((val & 0xF) * 0x2000) & ramMask;
		} else {
			// 6000-7FFF - ???
		}
	};
	void WriteA(uint16_t addr, uint8_t val) override {
		if(ramEnable) ram[ramBank | (addr & 0x1FFF)] = val;
	};
};

}
