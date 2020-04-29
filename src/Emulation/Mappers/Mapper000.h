#pragma once
#include "Mapper.h"

class Mapper000 : public Mapper {
public:
	Mapper000(uint8_t prgBanks, uint8_t chrBanks) : Mapper(prgBanks, chrBanks) { }

	int cpuMapRead(uint16_t addr, uint32_t& mapped) override;
	bool cpuMapWrite(uint16_t addr, uint8_t data) override;
	bool ppuMapRead(uint16_t addr, uint32_t& mapped, bool readOnly) override;
	bool ppuMapWrite(uint16_t addr, uint8_t data) override;

	void SaveState(saver& saver) override {}
	void LoadState(saver& saver) override {}
};
