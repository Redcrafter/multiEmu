#pragma once
#include "MBC.h"

namespace Gameboy {

class NoMBC : public MBC {
  public:
	NoMBC(const std::vector<uint8_t>& rom, uint32_t ramSize) : MBC(rom, ramSize) {}
	~NoMBC() override = default;

	uint8_t CpuRead(uint16_t addr) const override;
	void CpuWrite(uint16_t addr, uint8_t val) override;
};

}
