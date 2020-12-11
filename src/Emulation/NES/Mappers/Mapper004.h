#pragma once
#include "Mapper.h"
#include "MemoryMapped.h"

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

		uint8_t reg = 0; // unsepcified
	} bankSelect;

	uint8_t regs[8]{ 0,2,4,5,6,7,0,1 };

	uint32_t prgBankOffset[4];
	uint32_t chrBankOffset[8];

	bool reloadIrq = false;
	bool irqEnable = false;
	bool lastA12 = false;
	uint8_t irqCounter = 0;
	uint8_t irqLatch = 0;

	uint8_t* prgRam = nullptr; // 0x2000;

	MemoryMapped* file = nullptr;

	// not implemented because of compatibility issue between MMC3 and MMC6 (http://wiki.nesdev.com/w/index.php/MMC3)
	// bool ramEnable;
public:
	Mapper004(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr);
	~Mapper004();

	int cpuRead(uint16_t addr, uint8_t& data) override;
	bool cpuWrite(uint16_t addr, uint8_t data) override;
	bool ppuRead(uint16_t addr, uint8_t& data, bool readOnly) override;
	bool ppuWrite(uint16_t addr, uint8_t data) override;

	void SaveState(saver& saver) override;
	void LoadState(saver& saver) override;

	void MapSaveRam(const std::string& path) override;
private:
	void UpdateRegs();
};
