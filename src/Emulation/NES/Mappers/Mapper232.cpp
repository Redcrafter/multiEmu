#include "Mapper232.h"

namespace Nes {

Mapper232::Mapper232(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr) : Mapper(prg, chr) {
	prgBanks[0] = 0;
	prgBanks[1] = 0xFF;
}

int Mapper232::cpuRead(uint16_t addr, uint8_t& data) {
	if(addr >= 0x8000) {
		data = prg[((addr & 0x3FFF) | (prgBanks[addr >> 14 & 1] * 0x4000)) & prgMask];
		return true;
	}

	return false;
}

bool Mapper232::cpuWrite(uint16_t addr, uint8_t data) {
	if(addr >= 0x8000) {
		switch(addr & 0x4000) {
			case 0x0000:
				prgBanks[0] = (data >> 1 & 0xC) | (prgBanks[0] & 3);
				prgBanks[1] = (data >> 1 & 0xC) | 3;
				break;
			case 0x4000:
				prgBanks[0] = (prgBanks[0] & 0xC) | (data & 3);
				prgBanks[1] = (prgBanks[1] & 0xC) | 3;
				break;
		}
	}

	return false;
}

bool Mapper232::ppuRead(uint16_t addr, uint8_t& data, bool readOnly) {
	if(addr < 0x2000 && !chr.empty()) {
		data = chr[addr & 0x1FFF];
		return true;
	}
	return false;
}

void Mapper232::SaveState(saver& saver) {
	saver << prgBanks[0];
	saver << prgBanks[1];
}

void Mapper232::LoadState(saver& saver) {
	saver >> prgBanks[0];
	saver >> prgBanks[1];
}

}
