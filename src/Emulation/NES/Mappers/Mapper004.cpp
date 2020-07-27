#include "Mapper004.h"
#include "../Cartridge.h"

Mapper004::Mapper004(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr): Mapper(prg, chr) {
	prgBankOffset[3] = prg.size() - 0x2000; // last bank
	UpdateRegs();
}

int Mapper004::cpuRead(uint16_t addr, uint8_t& data) {
	if(addr < 0x8000) {
		if(addr >= 0x6000) {
			data = prgRam[addr & 0x1FFF];
			return true;
		}
	} else {
		data = prg[((addr & 0x1FFF) | prgBankOffset[(addr >> 13) & 0b11]) & prgMask];
		return true;
	}

	return false;
}

bool Mapper004::cpuWrite(uint16_t addr, uint8_t data) {
	if(addr < 0x8000) {
		if(addr >= 0x6000) { // && ramEnable
			prgRam[addr & 0x1FFF] = data;
			return true;
		}

		return false;
	}

	switch(addr & 0xE001) {
		case 0x8000: {
			bankSelect.reg = data;
			UpdateRegs();
			break;
		}
		case 0x8001:
			// logger.Log("%i = %i\n", bankSelect.bankNumber, data);
			regs[bankSelect.bankNumber] = data;
			UpdateRegs();
			break;
		case 0xA000:
			if(mirror != MirrorMode::FourScreen) {
				mirror = data & 1 ? MirrorMode::Horizontal : MirrorMode::Vertical;
			}
			break;
		case 0xA001:
			// ramEnable = data & 0x80;
			break;
		case 0xC000:
			irqLatch = data;
			break;
		case 0xC001:
			reloadIrq = true;
			break;
		case 0xE000:
			Irq = false;
			irqEnable = false;
			break;
		case 0xE001:
			irqEnable = true;
			break;
	}

	return false;
}

bool Mapper004::ppuRead(uint16_t addr, uint8_t& data, bool readOnly) {
	if(!readOnly) {
		if(!lastA12 && (addr & 0x1000)) {
			if(irqCounter == 0 || reloadIrq) {
				reloadIrq = false;
				irqCounter = irqLatch;
			} else {
				irqCounter--;
			}

			if(irqCounter == 0) {
				Irq = irqEnable;
			}
		}
		lastA12 = addr & 0x1000;
	}

	if(addr < 0x2000 && !chr.empty()) {
		data = chr[((addr & 0x03FF) | chrBankOffset[(addr >> 10) & 7]) & chrMask];
		return true;
	}
	return false;
}

bool Mapper004::ppuWrite(uint16_t addr, uint8_t data) {
	if(!lastA12 && (addr & 0x1000)) {
		if(irqCounter == 0 || reloadIrq) {
			reloadIrq = false;
			irqCounter = irqLatch;
		} else {
			irqCounter--;
		}

		if(irqCounter == 0) {
			Irq = irqEnable;
		}
	}
	lastA12 = addr & 0x1000;

	return false;
}

void Mapper004::SaveState(saver& saver) {
	saver << bankSelect.reg;

	saver.Write(prgBankOffset, 4);
	saver.Write(chrBankOffset, 8);

	saver << reloadIrq;
	saver << irqEnable;
	saver << lastA12;
	saver << irqCounter;
	saver << irqLatch;

	saver.Write(prgRam, 0x2000);
	// saver << ramEnable;
}

void Mapper004::LoadState(saver& saver) {
	saver >> bankSelect.reg;

	saver.Read(prgBankOffset, 4);
	saver.Read(chrBankOffset, 8);

	saver >> reloadIrq;
	saver >> irqEnable;
	saver >> lastA12;
	saver >> irqCounter;
	saver >> irqLatch;

	saver.Read(prgRam, 0x2000);
	// saver >> ramEnable;
}

void Mapper004::UpdateRegs() {
	prgBankOffset[1] = regs[7] * 0x2000;

	if(bankSelect.P) {
		prgBankOffset[0] = prg.size() - 0x4000; // second last bank
		prgBankOffset[2] = regs[6] * 0x2000;
	} else {
		prgBankOffset[2] = prg.size() - 0x4000; // second last bank
		prgBankOffset[0] = regs[6] * 0x2000;
	}

	if(bankSelect.C) {
		chrBankOffset[0] = regs[2] * 0x400;
		chrBankOffset[1] = regs[3] * 0x400;
		chrBankOffset[2] = regs[4] * 0x400;
		chrBankOffset[3] = regs[5] * 0x400;

		chrBankOffset[4] = (regs[0] & ~1) * 0x400;
		chrBankOffset[5] = (regs[0] & ~1) * 0x400 + 0x400;

		chrBankOffset[6] = (regs[1] & ~1) * 0x400;
		chrBankOffset[7] = (regs[1] & ~1) * 0x400 + 0x400;
	} else {
		chrBankOffset[4] = regs[2] * 0x400;
		chrBankOffset[5] = regs[3] * 0x400;
		chrBankOffset[6] = regs[4] * 0x400;
		chrBankOffset[7] = regs[5] * 0x400;

		chrBankOffset[0] = (regs[0] & ~1) * 0x400;
		chrBankOffset[1] = (regs[0] & ~1) * 0x400 + 0x400;

		chrBankOffset[2] = (regs[1] & ~1) * 0x400;
		chrBankOffset[3] = (regs[1] & ~1) * 0x400 + 0x400;
	}
}

void Mapper004::SaveRam(saver& saver) {
	saver.Write(prgRam, sizeof(prgRam));
}

void Mapper004::LoadRam(saver& saver) {
	saver.Read(prgRam, sizeof(prgRam));
}
