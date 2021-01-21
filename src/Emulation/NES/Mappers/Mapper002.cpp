#include "Mapper002.h"

namespace Nes {

Mapper002::Mapper002(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr) : Mapper(prg, chr) {
	if(!chr.empty()) {
		throw std::invalid_argument("Chr not allowed");
	}
}

int Mapper002::cpuRead(uint16_t addr, uint8_t& data) {
	if(addr >= 0x8000) {
		if(addr >= 0xC000) {
			data = prg[((addr & 0x3FFF) | (prg.size() - 0x4000)) & prgMask];
		} else {
			data = prg[((addr & 0x3FFF) | (selectedBank * 0x4000)) & prgMask];
		}

		return true;
	}

	return false;
}

bool Mapper002::cpuWrite(uint16_t addr, uint8_t data) {
	if(addr >= 0x8000) {
		selectedBank = data;
	}

	return false;
}

bool Mapper002::ppuRead(uint16_t addr, uint8_t& data, bool readOnly) {
	return false;
}

void Mapper002::SaveState(saver& saver) {
	saver << selectedBank;
}

void Mapper002::LoadState(saver& saver) {
	saver >> selectedBank;
}

}
