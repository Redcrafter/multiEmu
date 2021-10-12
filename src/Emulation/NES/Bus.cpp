#include "Bus.h"
#include "StandardController.h"

namespace Nes {
Bus::Bus() : cpu(mos6502(this)), apu(RP2A03(this)) {}

void Bus::InsertCartridge(std::shared_ptr<Mapper>& cartridge) {
	this->cartridge = cartridge;
	ppu.cartridge = cartridge;
}

void Bus::HardReset() {
	cpu.HardReset();
	ppu.HardReset();
	// apu.HardReset();

	irqDelay = false;
	for(unsigned char& i : CpuRam) {
		i = 0;
	}
	cpuOpenBus = 0;

	dmaPage = 0;
	dmaAddr = 0;
	dmaData = 0;

	dmaTransfer = false;
	dmaDummy = true;

	systemClockCounter = 0;
	CpuStall = 0;
}

void Bus::Reset() {
	cpu.Reset();
	ppu.Reset();
	apu.Reset();

	systemClockCounter = 0;
	dmaPage = 0;
	dmaAddr = 0;
	dmaData = 0;

	dmaTransfer = false;
	dmaDummy = true;

	irqDelay = 0;
}
// TODO: static int last4017 = 0; why is this even here?

void Bus::Clock() {
	ppu.Clock();

	if(systemClockCounter % 3 == 0) {
		// last4017++;
		apu.Clock();

		if(dmaTransfer) {
			if(dmaDummy) {
				if(systemClockCounter % 2 == 1) {
					dmaDummy = false;
				}
			} else {
				if(systemClockCounter % 2 == 0) {
					dmaData = CpuRead(dmaPage << 8 | dmaAddr);
				} else {
					ppu.pOAM[(ppu.oamAddr + dmaAddr) & 0xFF] = dmaData;
					dmaAddr++;

					if(dmaAddr == 0) {
						dmaTransfer = false;
						dmaDummy = true;
					}
				}
			}
		} else {
			if(CpuStall) {
				CpuStall--;
			}
			cpu.Clock();
			cartridge->CpuClock();

			cpu.IRQ = irqDelay || cartridge->Irq;
			irqDelay = apu.GetIrq();
		}
	}

	if(ppu.nmi) {
		if(ppu.nmi == 5) {
			ppu.nmi = 0;
			cpu.Nmi();
		} else if(!ppu.Control.enableNMI && (ppu.nmi <= 2)) {
			ppu.nmi = 0;
		} else {
			ppu.nmi++;
		}
	}

	systemClockCounter++;
}

void Bus::CpuWrite(uint16_t addr, uint8_t data) {
	if(cartridge->cpuWrite(addr, data)) {
		// 0x4020-0xFFFF
	} else if(addr < 0x2000) {
		// 0x0000 - 0x1FFF
		CpuRam[addr & 0x07FF] = data;
	} else if(addr < 0x4000) {
		// 0x2000 - 0x3FFF
		ppu.cpuWrite(addr & 0x2007, data);
	} else if(addr < 0x4016) {
		if(addr == 0x4014) {
			dmaPage = data;
			dmaAddr = 0;
			dmaTransfer = true;
			// ppu.cpuWrite(addr, data);
		} else {
			apu.CpuWrite(addr, data);
		}
	} else if(addr == 0x4016) {
		if(controller1 != nullptr) {
			controller1->CpuWrite(addr, data);
		}
		if(controller2 != nullptr) {
			controller2->CpuWrite(addr, data);
		}
	} else if(addr == 0x4017) {
		// last4017 = 0;
		apu.CpuWrite(addr, data);
	} else {
		apu.CpuWrite(addr, data);
	}
}

uint8_t Bus::CpuRead(uint16_t addr, bool readOnly) {
	uint8_t data = 0;

	if(cartridge->cpuRead(addr, data)) {

	} else if(addr < 0x2000) {
		data = CpuRam[addr & 0x07FF];
	} else if(addr < 0x4000) {
		data = ppu.cpuRead(addr & 0x2007, readOnly);
	} else {
		switch(addr) {
			case 0x4015:
				data = apu.ReadStatus(readOnly);
				break;
			case 0x4016:
				if(controller1 != nullptr) {
					data = (cpuOpenBus & 0xE0) | (controller1->CpuRead(addr, readOnly) & 0x1F);
				}
				break;
			case 0x4017:
				if(controller2 != nullptr) {
					data = (cpuOpenBus & 0xE0) | (controller2->CpuRead(addr, readOnly) & 0x1F);
				}
				break;
			default: data = cpuOpenBus;
		}
	}

	cpuOpenBus = data;
	return data;
}

void Bus::SaveState(saver& saver) {
	cpu.SaveState(saver);
	ppu.SaveState(saver);
	apu.SaveState(saver);

	cartridge->SaveState(saver);

	saver << CpuRam;
	saver << dmaPage;
	saver << dmaAddr;
	saver << dmaData;

	saver << dmaTransfer;
	saver << dmaDummy;

	saver << systemClockCounter;
}

void Bus::LoadState(saver& saver) {
	cpu.LoadState(saver);
	ppu.LoadState(saver);
	apu.LoadState(saver);

	cartridge->LoadState(saver);

	saver >> CpuRam;
	saver >> dmaPage;
	saver >> dmaAddr;
	saver >> dmaData;

	saver >> dmaTransfer;
	saver >> dmaDummy;

	saver >> systemClockCounter;
}
}