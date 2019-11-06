#pragma once
#include "Mapper.h"

class Mapper004 : public Mapper {
private:
	union {
		struct {
			uint8_t bankNumber : 3;
			uint8_t unused : 2;
			bool M : 1;
			bool P : 1;
			bool C : 1;
		};

		uint8_t reg;
	} bankSelect;

	uint32_t prgBankOffset[4];
	uint32_t chrBankOffset[8];

	bool reloadIrq;
	bool irqEnable;
	bool lastA12 = false;
	uint8_t irqCounter;
	uint8_t irqLatch;
	
	uint8_t prgRam[0x2000];
	bool ramEnable;
public:
	Mapper004(uint8_t prgBanks, uint8_t chrBanks);
	
	int cpuMapRead(uint16_t addr, uint32_t& mapped) override;
	bool cpuMapWrite(uint16_t addr, uint8_t data) override;
	bool ppuMapRead(uint16_t addr, uint32_t& mapped) override;
	bool ppuMapWrite(uint16_t addr, uint8_t data) override;

	void SaveState(saver& saver) override;
	void LoadState(saver& saver) override;
};
