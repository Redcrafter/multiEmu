#pragma once
#include "Mapper.h"

namespace Nes {

class Mapper011 : public Mapper {
  private:
	uint8_t prgBank = 0;
	uint8_t chrBank = 0;

  public:
	Mapper011(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr);
	~Mapper011() override = default;

	int cpuRead(uint16_t addr, uint8_t& data) override;
	bool cpuWrite(uint16_t addr, uint8_t data) override;
	bool ppuRead(uint16_t addr, uint8_t& data, bool readOnly) override;

	void SaveState(saver& saver) override;
	void LoadState(saver& saver) override;
};

}
