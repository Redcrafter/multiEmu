#include "Mapper065.h"

namespace Nes {

Mapper065::Mapper065(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr) : Mapper(prg, chr) {
	prgBankOffset[0] = 0;
	prgBankOffset[1] = 1;
	prgBankOffset[2] = 0xFE;
	prgBankOffset[3] = 0xFF;
}

int Mapper065::cpuRead(uint16_t addr, uint8_t& data) {
	if(addr >= 0x8000) {
		data = prg[((addr & 0x1FFF) | (prgBankOffset[(addr >> 13) & 3] << 13)) & prgMask];
		return true;
	}

	return false;
}

bool Mapper065::cpuWrite(uint16_t addr, uint8_t data) {
	switch(addr) {
		case 0x8000:
			prgBankOffset[0] = data;
			break;
		case 0xA000:
			prgBankOffset[1] = data;
			break;
		case 0xC000:
			prgBankOffset[2] = data;
			break;
		case 0xB000:
		case 0xB001:
		case 0xB002:
		case 0xB003:
		case 0xB004:
		case 0xB005:
		case 0xB006:
		case 0xB007:
			chrBankOffset[addr & 7] = data;
			break;
		case 0x9001:
			if(data >> 7) {
				mirror = MirrorMode::Horizontal;
			} else {
				mirror = MirrorMode::Vertical;
			}
			break;
		case 0x9003:
			irqEnable = data >> 7;
			Irq = false;
			break;
		case 0x9004:
			irqCounter = irqReload;
			Irq = false;
			break;
		case 0x9005:
			irqReload = (irqReload & 0x00FF) | (data << 8);
			break;
		case 0x9006:
			irqReload = (irqReload & 0xFF00) | data;
			break;
	}

	return false;
}

bool Mapper065::ppuRead(uint16_t addr, uint8_t& data, bool readOnly) {
	if(addr < 0x2000) {
		data = chr[((addr & 0x3FF) | (chrBankOffset[addr >> 10] << 10)) & chrMask];
		return true;
	}
	return false;
}

bool Mapper065::ppuWrite(uint16_t addr, uint8_t data) {
	return false;
}

void Mapper065::CpuClock() {
	if(irqEnable && irqCounter != 0) {
		irqCounter--;
		if(irqCounter == 0) {
			Irq = true;
		}
	}
}

void Mapper065::SaveState(saver& saver) {}
void Mapper065::LoadState(saver& saver) {}

}
