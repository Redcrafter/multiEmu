#include "Mapper002.h"

int Mapper002::cpuMapRead(uint16_t addr, uint32_t& mapped) {
	if(addr >= 0x8000) {
		uint8_t bank = 0;
		if(addr >= 0xC000) {
			// last bank
			bank = prgBanks - 1;
		} else {
			bank = selectedBank;
		}
		
		mapped = (addr & 0x3FFF) + (bank * 0x4000);
		return true;
	}
	
	return false;
}

bool Mapper002::cpuMapWrite(uint16_t addr, uint8_t data) {
	if(addr >= 0x8000) {
		selectedBank = data;
	}

	return false;
}

bool Mapper002::ppuMapRead(uint16_t addr, uint32_t& mapped) {
	return false;
}

bool Mapper002::ppuMapWrite(uint16_t addr, uint8_t data) {
	return false;
}

void Mapper002::SaveState(saver& saver) {
	saver << selectedBank;
}

void Mapper002::LoadState(saver& saver) {
	saver >> selectedBank;
}
