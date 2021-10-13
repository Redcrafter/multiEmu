#pragma once
#include "Mapper.h"
#include "../../../MemoryMapped.h"

namespace Nes {

class Mapper001 : public Mapper {
  private:
	bool lastWrite = false;

	uint8_t Control = 0b01100;
	uint8_t shiftRegister = 0b100000;

	uint8_t chrBank0 = 0, chrBank1 = 0, prgBank = 0;

	uint32_t prgBankOffset[2];
	uint32_t chrBankOffset[2];

	bool ramEnable = true;
	uint8_t* prgRam = nullptr;

	MemoryMapped* file = nullptr;

  public:
	Mapper001(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr);
	~Mapper001() override;

	int cpuRead(uint16_t addr, uint8_t& data) override;
	bool cpuWrite(uint16_t addr, uint8_t data) override;
	bool ppuRead(uint16_t addr, uint8_t& mapped, bool readOnly) override;

	void SaveState(saver& saver) override;
	void LoadState(saver& saver) override;

	void MapSaveRam(const std::string& path) override;
};

}
