#pragma once
#include "MBC.h"

namespace Gameboy {

class MBC1 : public MBC {
  private:
	bool ramEnable = false;
	uint8_t reg1 = 1;
	uint8_t extendedBank = 0;
	bool mode = false;

	uint32_t romBank0 = 0;
	uint32_t romBank1 = 1 << 14;
	uint32_t ramBank = 0;

  public:
	MBC1(const std::vector<uint8_t>& rom, uint32_t ramSize) : MBC(rom, ramSize) {}
	~MBC1() override = default;

	uint8_t CpuRead(uint16_t addr) const override;
	void CpuWrite(uint16_t addr, uint8_t val) override;

  private:
	void updateBanks();
};

}
