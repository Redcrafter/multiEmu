#include "Mapper004.h"
#include "../Cartridge.h"

Mapper004::Mapper004(uint8_t prgBanks, uint8_t chrBanks): Mapper(prgBanks, chrBanks) {
	prgBankOffset[0] = (prgBanks * 2 - 2) * 0x2000;
	prgBankOffset[2] = (prgBanks * 2 - 2) * 0x2000;
	prgBankOffset[3] = (prgBanks * 2 - 1) * 0x2000;

	for(int i = 0; i < 8; ++i) {
		chrBankOffset[i] = i * 0x400;
	}

	irqEnable = false;
}

int Mapper004::cpuMapRead(uint16_t addr, uint32_t& mapped) {
	if(addr < 0x8000) {
		if(addr >= 0x6000) {
			mapped = prgRam[addr & 0x1FFF];
			return 2;
		}
		return 0;
	}

	mapped = (addr & 0x1FFF) | prgBankOffset[(addr >> 13) & 0b11];
	return 1;
}

bool Mapper004::cpuMapWrite(uint16_t addr, uint8_t data) {
	if(addr < 0x8000) {
		if(addr >= 0x6000 && ramEnable) {
			prgRam[addr & 0x1FFF] = data;

			return true;
		}

		return false;
	}

	switch(addr & 0xE001) {
		case 0x8000: {
			bool old = bankSelect.M;
			bankSelect.reg = data;

			if(old != bankSelect.M) {
				uint32_t temp = prgBankOffset[0];
				prgBankOffset[0] = prgBankOffset[2];
				prgBankOffset[2] = temp;
			}
			break;
		}
		case 0x8001:
			switch(bankSelect.bankNumber) {
				case 0:
					data = data & ~1;
					if(!bankSelect.C) {
						chrBankOffset[0] = (data) * 0x400;
						chrBankOffset[1] = (data + 1) * 0x400;
					} else {
						chrBankOffset[4] = data * 0x400;;
						chrBankOffset[5] = (data + 1) * 0x400;
					}
					break;
				case 1:
					data = data & ~1;
					if(!bankSelect.C) {
						chrBankOffset[2] = data * 0x400;
						chrBankOffset[3] = (data + 1) * 0x400;
					} else {
						chrBankOffset[6] = data * 0x400;;
						chrBankOffset[7] = (data + 1) * 0x400;
					}
					break;
				case 2:
					if(!bankSelect.C) {
						chrBankOffset[4] = data * 0x400;
					} else {
						chrBankOffset[0] = data * 0x400;
					}
					break;
				case 3:
					if(!bankSelect.C) {
						chrBankOffset[5] = data * 0x400;
					} else {
						chrBankOffset[1] = data * 0x400;
					}
					break;
				case 4:
					if(!bankSelect.C) {
						chrBankOffset[6] = data * 0x400;
					} else {
						chrBankOffset[2] = data * 0x400;
					}
					break;
				case 5:
					if(!bankSelect.C) {
						chrBankOffset[7] = data * 0x400;
					} else {
						chrBankOffset[3] = data * 0x400;
					}
					break;
				case 6:
					if(!bankSelect.M) {
						prgBankOffset[0] = data * 0x2000;
					} else {
						prgBankOffset[2] = data * 0x2000;
					}
					break;
				case 7:
					prgBankOffset[1] = data * 0x2000;
					break;
			}
			break;
		case 0xA000:
			mirror = data & 1 ? MirrorMode::Horizontal : MirrorMode::Vertical;
			break;
		case 0xA001:
			ramEnable = data & 0x80;
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
			// TODO: acknowledge any pending interrupts
			break;
		case 0xE001:
			irqEnable = true;
			break;
	}

	return false;
}

bool Mapper004::ppuMapRead(uint16_t addr, uint32_t& mapped, bool readOnly) {
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
	
	if(addr >= 0x2000) {
		return false;
	}

	mapped = (addr & 0x03FF) | chrBankOffset[(addr >> 10) & 7];
	return true;
}

bool Mapper004::ppuMapWrite(uint16_t addr, uint8_t data) {
	if(!lastA12 && (addr & 0x1000)) {
		// A12

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
	saver.Write(reinterpret_cast<char*>(this), sizeof(Mapper004State));
}

void Mapper004::LoadState(saver& saver) {
	saver.Read(reinterpret_cast<char*>(this), sizeof(Mapper004State));
}
