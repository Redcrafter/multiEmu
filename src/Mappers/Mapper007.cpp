#include "Mapper007.h"
#include "../Emulation/Cartridge.h"

Mapper007::Mapper007(uint8_t prgBanks, uint8_t chrBanks) : Mapper(prgBanks, chrBanks) {
	prgBank = 0;
}

int Mapper007::cpuMapRead(uint16_t addr, uint32_t& mapped) {
	mapped = (addr & 0x7FFF) | (prgBank * 0x8000);

	return addr >= 0x8000;
}

bool Mapper007::cpuMapWrite(uint16_t addr, uint8_t data) {
	if(addr < 0x8000) {
		return false;
	}

	prgBank = data & 0b111;

	if(data & 0x10) {
		mirror = MirrorMode::OnescreenHi;
	} else {
		mirror = MirrorMode::OnescreenLo;
	}

	return false;
}

bool Mapper007::ppuMapRead(uint16_t addr, uint32_t& mapped) {
	if(addr < 0x2000) {
		mapped = addr;
		return true;
	}
	return false;
}

bool Mapper007::ppuMapWrite(uint16_t addr, uint8_t data) {
	return false;
}

void Mapper007::SaveState(saver& saver) {
	saver << prgBank;
}

void Mapper007::LoadState(saver& saver) {
	saver >> prgBank;
}
