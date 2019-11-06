#pragma once
#include "Mapper.h"

class Mapper001 : public Mapper {
public:
	Mapper001(uint8_t prgBanks, uint8_t chrBanks);
	
	int cpuMapRead(uint16_t addr, uint32_t& mapped) override;
	bool cpuMapWrite(uint16_t addr, uint8_t data) override;
	bool ppuMapRead(uint16_t addr, uint32_t& mapped) override;
	bool ppuMapWrite(uint16_t addr, uint8_t data) override;

	void SaveState(saver& saver) override;
	void LoadState(saver& saver) override;
private:
	uint8_t writeState = 0;
	uint8_t Control = 0b01100;
	uint8_t shiftRegister = 0;

	uint8_t chrBank0 = 0, chrBank1 = 0, prgBank = 0;

	uint32_t prgBankOffset[2];
	uint32_t chrBankOffset[2];
};
