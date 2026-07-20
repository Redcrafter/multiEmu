#pragma once
#include "MBC.h"

namespace Gameboy {

class NoMBC final : public MBC {
  public:
	NoMBC(const std::vector<uint8_t>& rom, uint32_t ramSize, bool hasBattery) : MBC(rom, ramSize, hasBattery) {}
	~NoMBC() override = default;

	uint8_t Read0(uint16_t addr) const override { return rom[addr & 0x3FFF]; }
	uint8_t Read4(uint16_t addr) const override { return rom[addr & romMask]; }
	uint8_t ReadA(uint16_t addr) const override { return (ramMask != -1u) ? ram[addr & ramMask] : 0xFF; }

	void Write0(uint16_t addr, uint8_t val) override {}
	void Write4(uint16_t addr, uint8_t val) override {}
	void WriteA(uint16_t addr, uint8_t val) override {
		if(ramMask != -1u) ram[addr & ramMask] = val;
	};

	void SaveState(nlohmann::json& saver) const override {
		saver["ram"] = std::span(ram, ramMask + 1);
	}
	void LoadState(const nlohmann::json& saver) override {
		auto dat = saver["ram"].get<std::vector<uint8_t>>();
		assert(dat.size() == ramMask + 1);
		std::memcpy(ram, dat.data(), ramMask + 1);
	}
};

}
