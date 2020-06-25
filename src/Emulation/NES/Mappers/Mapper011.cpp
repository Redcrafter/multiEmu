#include "Mapper011.h"

Mapper011::Mapper011(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr) : Mapper(prg, chr) { }

int Mapper011::cpuRead(uint16_t addr, uint8_t& data) {
	if(addr >= 0x8000) {
		data = prg[((addr & 0x7FFF) | (prgBank * 0x8000)) & prgMask];
		return true;
	}

	return false;
}

bool Mapper011::cpuWrite(uint16_t addr, uint8_t data) {
	if(addr >= 0x8000) {
		prgBank = data & 3;
		chrBank = data >> 4;
	}

	return false;
}

bool Mapper011::ppuRead(uint16_t addr, uint8_t& data, bool readOnly) {
	if(addr < 0x2000 && !chr.empty()) {
		data = chr[((addr & 0x1FFF) | (chrBank * 0x2000)) & chrMask];
		return true;
	}
	return false;
}

void Mapper011::SaveState(saver& saver) {
	saver << prgBank;
	saver << chrBank;
}

void Mapper011::LoadState(saver& saver) {
	saver >> prgBank;
	saver >> chrBank;
}
