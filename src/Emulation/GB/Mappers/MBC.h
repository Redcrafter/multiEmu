#pragma once
#include <cassert>
#include <cstdint>
#include <vector>

#include "../../../saver.h"

namespace Gameboy {

static bool checkLogo(const uint8_t* rom) {
	const uint8_t logo[] = {
		0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
		0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
		0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
	};

	for(size_t i = 0; i < sizeof(logo); i++) {
		if(rom[i] != logo[i]) {
			return false;
		}
	}
	return true;
}

class MBC {
	friend class Core;

  protected:
	std::vector<uint8_t> rom;
	std::vector<uint8_t> ram;

	uint32_t romMask;
	uint32_t ramMask;

	bool hasBattery;

  public:
	MBC() {};
	MBC(const std::vector<uint8_t>& rom, uint32_t ramSize, bool hasBattery) : rom(rom), hasBattery(hasBattery) {
		ram.resize(ramSize);
		// should be something like 0x1000 - 1 = 0xFFF
		romMask = rom.size() - 1;
		ramMask = ramSize - 1;
	}
	virtual ~MBC() = default;

	virtual uint8_t Read0(uint16_t addr) const = 0;
	virtual uint8_t Read4(uint16_t addr) const = 0;
	virtual uint8_t ReadA(uint16_t addr) const = 0;

	virtual void Write0(uint16_t addr, uint8_t val) = 0;
	virtual void Write4(uint16_t addr, uint8_t val) = 0;
	virtual void WriteA(uint16_t addr, uint8_t val) = 0;

	virtual void SaveState(saver& saver) = 0;
	virtual void LoadState(saver& saver) = 0;
};

}
