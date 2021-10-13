#pragma once
#include "Mapper.h"

namespace Nes {

class Mapper003 : public Mapper {
  public:
	Mapper003(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr);
	~Mapper003() override = default;

	int cpuRead(uint16_t addr, uint8_t& data) override;
	bool cpuWrite(uint16_t addr, uint8_t data) override;
	bool ppuRead(uint16_t addr, uint8_t& data, bool readOnly) override;

	void SaveState(saver& saver) override;
	void LoadState(saver& saver) override;

  private:
	uint8_t selectedBank = 0;
};

}
