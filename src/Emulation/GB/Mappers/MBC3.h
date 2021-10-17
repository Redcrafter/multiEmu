#pragma once
#include "MBC.h"

namespace Gameboy {

class MBC3 final : public MBC {
  private:
	uint32_t romBank1 = 0x4000;
	uint8_t aSelect = 0;

	bool ramEnable = false;
	bool hasTimer;

  public:
	MBC3(const std::vector<uint8_t>& rom, uint32_t ramSize, bool hasBattery, bool hasTimer) : MBC(rom, ramSize, hasBattery), hasTimer(hasTimer) {}
	~MBC3() override = default;

	uint8_t Read0(uint16_t addr) const override { return rom[addr & 0x3FFF]; }
	uint8_t Read4(uint16_t addr) const override { return rom[romBank1 | (addr & 0x3FFF)]; };
	uint8_t ReadA(uint16_t addr) const override {
		if(aSelect < 4 && ramEnable) {
			return ram[((aSelect << 13) | (addr & 0x1FFF)) & ramMask];
		}

		if(hasTimer && aSelect >= 8 && aSelect <= 12) {
			assert(false);
		}

		return 0xFF;
	};

	void Write0(uint16_t addr, uint8_t val) override {
		if(addr < 0x2000) {
			// 0000-1FFF - RAM Enable
			ramEnable = !ram.empty() && (val & 0xF) == 0xA;
		} else {
			// 2000-3FFF - ROM Bank Number
			romBank1 = (0x4000 * std::max(val & 0x8F, 1)) & romMask;
		}
	};
	void Write4(uint16_t addr, uint8_t val) override {
		if(addr < 0x6000) {
			// 4000-5FFF - RAM Bank Number - or - Upper Bits of ROM Bank Number
			aSelect = (val & 3) << 5;
		} else {
			// 6000-7FFF - Banking Mode Select
			if(hasTimer) assert(false);
		}
	};
	void WriteA(uint16_t addr, uint8_t val) override {
		if(aSelect < 4 && ramEnable) {
			ram[((aSelect << 13) | (addr & 0x1FFF)) & ramMask] = val;
		}
	};

	void SaveState(saver& saver) override {
		saver.write(ram.data(), ram.size());
		saver << romBank1;
		saver << aSelect;
		saver << ramEnable;
	};
	void LoadState(saver& saver) override {
		saver.read(ram.data(), ram.size());
		saver >> romBank1;
		saver >> aSelect;
		saver >> ramEnable;
	};
};

}
