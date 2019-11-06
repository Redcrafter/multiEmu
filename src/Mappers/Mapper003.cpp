#include "Mapper003.h"

int Mapper003::cpuMapRead(uint16_t addr, uint32_t& mapped) {
	if(addr >= 0x8000) {
		mapped = addr & (prgBanks > 1 ? 0x7FFF : 0x3FFF);
		return true;
	}
	
	return false;
}

bool Mapper003::cpuMapWrite(uint16_t addr, uint8_t data) {
	if(addr >= 0x8000) {
		selectedBank = data & 3;
	}

	return false;
}

bool Mapper003::ppuMapRead(uint16_t addr, uint32_t& mapped) {
	if(addr < 0x2000) {
		mapped = (addr & 0x1FFF) + (selectedBank * 0x2000);
		
		return true;
	}
	return false;
}

bool Mapper003::ppuMapWrite(uint16_t addr, uint8_t data) {
	return false;
}

void Mapper003::SaveState(saver& saver) {
	saver << selectedBank;
}

void Mapper003::LoadState(saver& saver) {
	saver >> selectedBank;
}
