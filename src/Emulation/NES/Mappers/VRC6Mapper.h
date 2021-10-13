#pragma once
#include "Mapper.h"

namespace Nes {

class VRC6Mapper : public Mapper {
  private:
	int prgBanks;
	int chrBanks;

	bool swap;
	uint16_t prgBankOffset[4];
	uint16_t chrBankOffset[8];

	bool ramEnable = false;
	uint8_t prgRam[0x2000];

  public:
	VRC6Mapper(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr, bool swap);
	~VRC6Mapper() override = default;

	int cpuRead(uint16_t addr, uint8_t& data) override;
	bool cpuWrite(uint16_t addr, uint8_t data) override;
	bool ppuRead(uint16_t addr, uint8_t& data, bool readOnly) override;
	bool ppuWrite(uint16_t addr, uint8_t data) override;

	void SaveState(saver& saver) override {}
	void LoadState(saver& saver) override {}
};

}
