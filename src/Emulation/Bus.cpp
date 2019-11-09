#include "Bus.h"
#include "../Input/StandardController.h"
#include "../Mappers/Mapper004.h"

Bus::Bus(std::shared_ptr<Cartridge>& cartridge) : cartridge(cartridge), cpu(mos6502(this)), apu(RP2A03(this)) {
	ppu.cartridge = cartridge;

	for(unsigned char& i : CpuRam) {
		i = 0;
	}
}

void Bus::InsertCartridge(std::shared_ptr<Cartridge>& cartridge) {
	this->cartridge = cartridge;
	ppu.cartridge = cartridge;
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
}
static int irqDelay = 0;
static int last4017 = 0;

void Bus::Clock() {
	ppu.Clock();

	if(systemClockCounter % 3 == 0) {
		last4017++;
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
			cpu.Clock();
			cpu.IRQ = irqDelay | cartridge->GetIrq();
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
		last4017 = 0;
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
					data = controller1->CpuRead(addr, readOnly);
				}
				break;
			case 0x4017:
				if(controller2 != nullptr) {
					data = controller2->CpuRead(addr, readOnly);
				}
				break;
		}
	}

	return data;
}

void Bus::SaveState(saver& saver) {
	cpu.SaveState(saver);
	ppu.SaveState(saver);
	apu.SaveState(saver);

	cartridge->SaveState(saver);

	saver.Write(reinterpret_cast<char*>(&CpuRam), sizeof(CpuRam));
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
	
	saver.Read(reinterpret_cast<char*>(&CpuRam), sizeof(CpuRam));
	saver >> dmaPage;
	saver >> dmaAddr;
	saver >> dmaData;

	saver >> dmaTransfer;
	saver >> dmaDummy;

	saver >> systemClockCounter;
}
