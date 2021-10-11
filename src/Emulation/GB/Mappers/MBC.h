#pragma once
#include <cassert>
#include <cstdint>
#include <vector>

namespace Gameboy {

class MBC {
	friend class GameboyColorCore;

  protected:
	std::vector<uint8_t> rom;
	std::vector<uint8_t> ram;

	uint32_t romMask;
	uint32_t ramMask;

  public:
	bool hasRam;
	bool hasBattery;

  public:
	MBC(const std::vector<uint8_t>& rom, uint32_t ramSize) : rom(rom) {
		ram.resize(ramSize);
		// should be something like 0x1000 - 1 = 0xFFF
		romMask = rom.size() - 1;
		ramMask = ramSize - 1;
	}

	uint8_t Bank0(uint16_t addr) const {
		return rom[addr];
	}

	virtual uint8_t CpuRead(uint16_t addr) const = 0;
	virtual void CpuWrite(uint16_t addr, uint8_t val) = 0;
};

}
