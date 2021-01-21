#include "Mapper003.h"

namespace Nes {

Mapper003::Mapper003(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr) : Mapper(prg, chr) {
	if(this->prg.size() == 0x4000) {
		prgMask = 0x3FFF;
	} else if(this->prg.size() == 0x8000) {
		prgMask = 0x7FFF;
	} else {
		throw std::logic_error("Invalid prg size");
	}
}

int Mapper003::cpuRead(uint16_t addr, uint8_t& data) {
	if(addr >= 0x8000) {
		data = prg[addr & prgMask];
		return true;
	}

	return false;
}

bool Mapper003::cpuWrite(uint16_t addr, uint8_t data) {
	if(addr >= 0x8000) {
		selectedBank = data;
	}

	return false;
}

bool Mapper003::ppuRead(uint16_t addr, uint8_t& data, bool readOnly) {
	if(addr < 0x2000) {
		data = chr[((addr & 0x1FFF) | (selectedBank * 0x2000)) & chrMask];
		return true;
	}
	return false;
}

void Mapper003::SaveState(saver& saver) {
	saver << selectedBank;
}

void Mapper003::LoadState(saver& saver) {
	saver >> selectedBank;
}

}
