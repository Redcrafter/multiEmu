#pragma once
#include <cstdint>

// #include "cpu6502.h"
#include "ppu2C02.h"
#include "RP2A03.h"

#include "Cartridge.h"
#include "../Input/Controller.h"
#include "mos6502.h"

class Bus {
public:
	uint64_t systemClockCounter = 0;

	std::shared_ptr<Cartridge> cartridge;
	cpu6502 cpu;
	ppu2C02 ppu;
	RP2A03 apu;
	std::shared_ptr<Controller> controller1, controller2;
	
	Bus(std::shared_ptr<Cartridge>& cartridge);

	void InsertCartridge(std::shared_ptr<Cartridge>& cartridge);
	void Reset();
	void Clock();

	void CpuWrite(uint16_t addr, uint8_t data);
	uint8_t CpuRead(uint16_t addr);

	void SaveState(saver& saver);
	void LoadState(saver& saver);
private:
	uint8_t CpuRam[2 * 1024];

	uint8_t dmaPage = 0;
	uint8_t dmaAddr = 0;
	uint8_t dmaData = 0;

	bool dmaTransfer = false,
	     dmaDummy = true;
};
