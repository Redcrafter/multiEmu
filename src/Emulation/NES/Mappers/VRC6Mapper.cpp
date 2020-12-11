#include "VRC6Mapper.h"

VRC6Mapper::VRC6Mapper(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr, bool swap): Mapper(prg, chr), swap(swap) {
	prgBanks = prg.size() / 0x4000;
	chrBanks = chr.size() / 0x2000;

	prgBankOffset[3] = (prgBanks - 1) * 0x4000; // fixed last bank
}

int VRC6Mapper::cpuRead(uint16_t addr, uint8_t& data) {
	if(addr >= 0x8000) {
		data = prg[(addr & 0x1FFF) | (prgBankOffset[(addr >> 13) & 3])];
		return true;
	} else if(addr >= 0x6000 && ramEnable) {
		data = prgRam[addr & 0x1FFF];
		return true;
	}
	return false;
}

bool VRC6Mapper::cpuWrite(uint16_t addr, uint8_t data) {
	switch(addr & 0xF003) {
		case 0x8000:
		case 0x8001:
		case 0x8002:
		case 0x8003:
			prgBankOffset[0] = data * 0x4000;
			prgBankOffset[1] = data * 0x4000 + 0x2000;
			return true;
		case 0xC000:
		case 0xC001:
		case 0xC002:
		case 0xC003:
			prgBankOffset[2] = data * 0x2000;
			return true;
		case 0xB003:
			ramEnable = data >> 7;
			break;
		case 0xD000:
		case 0xD001:
		case 0xD002:
		case 0xD003:
		case 0xE000:
		case 0xE001:
		case 0xE002:
		case 0xE003:

			break;
	}
	return false;
}

bool VRC6Mapper::ppuRead(uint16_t addr, uint8_t& data, bool readOnly) {
	if(addr >= 0x2000 || chrBanks == 0)
		return false;

	// mapped = addr;
	return true;
}

bool VRC6Mapper::ppuWrite(uint16_t addr, uint8_t data) {
	return false;
}
