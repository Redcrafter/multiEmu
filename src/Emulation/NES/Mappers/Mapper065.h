#pragma once
#include "Mapper.h"

namespace Nes {

class Mapper065 : public Mapper {
  private:
	uint32_t prgBankOffset[4];
	uint32_t chrBankOffset[8] {};

	bool irqEnable = false;
	uint16_t irqCounter = 0;
	uint16_t irqReload = 0;

  public:
	Mapper065(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr);
	~Mapper065() override = default;

	int cpuRead(uint16_t addr, uint8_t& data) override;
	bool cpuWrite(uint16_t addr, uint8_t data) override;
	bool ppuRead(uint16_t addr, uint8_t& data, bool readOnly) override;
	bool ppuWrite(uint16_t addr, uint8_t data) override;

	void SaveState(saver& saver) override;
	void LoadState(saver& saver) override;

	void CpuClock() override;
};

}
