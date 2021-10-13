#pragma once
#include "Mapper.h"

namespace Nes {

class Mapper232 : public Mapper {
  private:
	uint8_t prgBanks[2];

  public:
	Mapper232(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr);
	~Mapper232() override = default;

	int cpuRead(uint16_t addr, uint8_t& data) override;
	bool cpuWrite(uint16_t addr, uint8_t data) override;
	bool ppuRead(uint16_t addr, uint8_t& data, bool readOnly) override;

	void SaveState(saver& saver) override;
	void LoadState(saver& saver) override;
};

}
