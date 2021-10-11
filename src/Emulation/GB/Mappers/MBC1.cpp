#include "MBC1.h"

namespace Gameboy {

uint8_t MBC1::CpuRead(uint16_t addr) const {
	switch(addr >> 14) {
		case 0: return rom[romBank0 | (addr & 0x3FFF)];
		case 1: return rom[romBank1 | (addr & 0x3FFF)];
		case 2:
			if(hasRam && ramEnable) {
				return ram[ramBank | (addr & 0x1FFF)];
			} else {
				return 0xFF;
			}
		default: assert(false); return 0;
	}
}

void MBC1::CpuWrite(uint16_t addr, uint8_t val) {
	switch(addr >> 13) {
		case 0:
			ramEnable = (val & 0xF) == 0xA;
			break;
		case 1:
			reg1 = val & 0x1F;
			if(reg1 == 0) {
				reg1 = 1;
			}
			updateBanks();
			break;
		case 2:
			extendedBank = val & 3;
			updateBanks();
			break;
		case 3:
			mode = val & 1;
			updateBanks();
			break;
		case 5:
		case 6:
			if(hasRam && ramEnable) {
				ram[ramBank | (addr & 0x1FFF)] = val;
			}
			break;
		default: assert(false);
	}
}

void MBC1::updateBanks() {
	if(mode == 0) {
		romBank0 = 0;
		ramBank = 0;
	} else {
		romBank0 = (extendedBank << 5 << 14) & romMask;
		ramBank = (extendedBank << 13) & ramMask;
	}
	romBank1 = (((extendedBank << 5) | reg1) << 14) & romMask;
}

}
