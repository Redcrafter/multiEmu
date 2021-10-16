#pragma once
#include "MBC.h"

namespace Gameboy {

class MBC1 final : public MBC {
  private:
	bool ramEnable = false;
	uint8_t reg1 = 1;
	uint8_t extendedBank = 0;
	bool mode = false;

	bool isMulti;

	uint32_t romBank0 = 0;
	uint32_t romBank1 = 0x4000;
	uint32_t ramBank = 0;

  public:
	MBC1(const std::vector<uint8_t>& rom, uint32_t ramSize, bool hasBattery) : MBC(rom, ramSize, hasBattery) {
		bool largeRom = rom.size() >= 0x100000;
		bool largeRam = ram.size() > 0x2000;
		isMulti = largeRom && checkLogo(&rom[0x40104]);

		assert((!largeRom && !largeRam) || (largeRom ^ largeRam));
	}
	~MBC1() override = default;

	uint8_t Read0(uint16_t addr) const override { return rom[romBank0 | (addr & 0x3FFF)]; }
	uint8_t Read4(uint16_t addr) const override { return rom[romBank1 | (addr & 0x3FFF)]; };
	uint8_t ReadA(uint16_t addr) const override {
		return ramEnable ? ram[ramBank | (addr & 0x1FFF)] : 0xFF;
	};

	void Write0(uint16_t addr, uint8_t val) override {
		if(addr < 0x2000) {
			// 0000-1FFF - RAM Enable
			ramEnable = !ram.empty() && (val & 0xF) == 0xA;
		} else {
			// 2000-3FFF - ROM Bank Number
			reg1 = std::max(val & 0x1F, 1);
			if(isMulti) reg1 &= 0xF;
			updateBanks();
		}
	};
	void Write4(uint16_t addr, uint8_t val) override {
		if(addr < 0x6000) {
			// 4000-5FFF - RAM Bank Number - or - Upper Bits of ROM Bank Number
			if(isMulti) {
				extendedBank = (val & 3) << 4;
			} else {
				extendedBank = (val & 3) << 5;
			}
			updateBanks();
		} else {
			// 6000-7FFF - Banking Mode Select
			mode = val & 1;
			updateBanks();
		}
	};
	void WriteA(uint16_t addr, uint8_t val) override {
		if(ramEnable) ram[ramBank | (addr & 0x1FFF)] = val;
	};

  private:
	void updateBanks() {
		if(mode == 0) {
			romBank0 = 0;
			ramBank = 0;
		} else {
			romBank0 = (0x4000 * extendedBank) & romMask;
			ramBank = (extendedBank << 8) & ramMask;
		}
		romBank1 = (0x4000 * (extendedBank | reg1)) & romMask;
	}
};

}
