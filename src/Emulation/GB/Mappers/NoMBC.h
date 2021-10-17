#pragma once
#include "MBC.h"

namespace Gameboy {

class NoMBC final : public MBC {
  public:
	NoMBC(const std::vector<uint8_t>& rom, uint32_t ramSize, bool hasBattery) : MBC(rom, ramSize, hasBattery) {}
	~NoMBC() override = default;

	uint8_t Read0(uint16_t addr) const override { return rom[addr & 0x3FFF]; }
	uint8_t Read4(uint16_t addr) const override { return rom[addr & romMask]; }
	uint8_t ReadA(uint16_t addr) const override { return !ram.empty() ? ram[addr & ramMask] : 0xFF; }

	void Write0(uint16_t addr, uint8_t val) override {};
	void Write4(uint16_t addr, uint8_t val) override {};
	void WriteA(uint16_t addr, uint8_t val) override {
		if(!ram.empty()) ram[addr & ramMask] = val;
	};

	void SaveState(saver& saver) override {
		saver.write(ram.data(), ram.size());
	};
	void LoadState(saver& saver) override {
		saver.read(ram.data(), ram.size());
	};
};

}
