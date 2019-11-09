#include "Mapper000.h"

int Mapper000::cpuMapRead(uint16_t addr, uint32_t& mapped) {
	if(addr >= 0x8000) {
		mapped = addr & (prgBanks > 1 ? 0x7FFF : 0x3FFF);
		return true;
	}
	return false;
}

bool Mapper000::cpuMapWrite(uint16_t addr, uint8_t data) {
	return false;
}

bool Mapper000::ppuMapRead(uint16_t addr, uint32_t& mapped, bool readOnly) {
	if(addr >= 0x2000 || chrBanks == 0)
		return false;
	
	mapped = addr;
	return true;
}

bool Mapper000::ppuMapWrite(uint16_t addr, uint8_t data) {
	return false;
}
