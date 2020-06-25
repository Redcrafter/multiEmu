#pragma once
#include <cstdint>

// #include "cpu6502.h"
#include "ppu2C02.h"
#include "RP2A03.h"

#include "Cartridge.h"
#include "Controller.h"
#include "mos6502.h"

class Bus {
private:
	bool irqDelay;

    uint8_t CpuRam[2 * 1024];
	uint8_t cpuOpenBus;

	uint8_t dmaPage;
	uint8_t dmaAddr;
	uint8_t dmaData;

	bool dmaTransfer;
	bool dmaDummy;
public:
	uint64_t systemClockCounter;
	int CpuStall;

	std::shared_ptr<Mapper> cartridge = nullptr;
	mos6502 cpu;
	ppu2C02 ppu;
	RP2A03 apu;
	std::shared_ptr<Controller> controller1, controller2;

	Bus();

	void InsertCartridge(std::shared_ptr<Mapper>& cartridge);

	void HardReset();
	void Reset();
	void Clock();

	void CpuWrite(uint16_t addr, uint8_t data);
	uint8_t CpuRead(uint16_t addr, bool readOnly = false);

	void SaveState(saver& saver);
	void LoadState(saver& saver);

	friend class NesCore;
};
