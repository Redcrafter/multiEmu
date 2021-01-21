#include "Mapper079.h"

namespace Nes {

Mapper079::Mapper079(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr) : Mapper(prg, chr) {}

int Mapper079::cpuRead(uint16_t addr, uint8_t& data) {
	if(addr >= 0x8000) {
		data = prg[((addr & 0x7FFF) | (prgBank * 0x8000)) & prgMask];
		return true;
	}

	return false;
}

bool Mapper079::cpuWrite(uint16_t addr, uint8_t data) {
	if((addr & 0b1110000100000000) == 0b0100000100000000) {
		prgBank = (data >> 3) & 1;
		chrBank = data & 7;
	}

	return false;
}

bool Mapper079::ppuRead(uint16_t addr, uint8_t& data, bool readOnly) {
	if(addr < 0x2000 && !chr.empty()) {
		data = chr[((addr & 0x1FFF) | (chrBank * 0x2000)) & chrMask];
		return true;
	}
	return false;
}

void Mapper079::SaveState(saver& saver) {
	saver << prgBank;
	saver << chrBank;
}

void Mapper079::LoadState(saver& saver) {
	saver >> prgBank;
	saver >> chrBank;
}

}
