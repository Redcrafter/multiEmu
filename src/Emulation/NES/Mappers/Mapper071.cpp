#include "Mapper071.h"

namespace Nes {

Mapper071::Mapper071(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr) : Mapper(prg, chr) {
	prgBanks[0] = 0;
	prgBanks[1] = 0xFF;
}

int Mapper071::cpuRead(uint16_t addr, uint8_t& data) {
	if(addr >= 0x8000) {
		data = prg[((addr & 0x3FFF) | (prgBanks[addr >> 14 & 1] * 0x4000)) & prgMask];
		return true;
	}

	return false;
}

bool Mapper071::cpuWrite(uint16_t addr, uint8_t data) {
	if(addr >= 0x8000) {
		switch(addr & 0x7000) {
			case 0x0000: // Mirroring (for Fire Hawk only!)
			case 0x1000:
				/* code */
				break;
			case 0x4000:
			case 0x5000:
			case 0x6000:
			case 0x7000:
				prgBanks[0] = data & 0xF;
				break;
		}
	}

	return false;
}

bool Mapper071::ppuRead(uint16_t addr, uint8_t& data, bool readOnly) {
	if(addr < 0x2000 && !chr.empty()) {
		data = chr[addr & 0x1FFF];
		return true;
	}
	return false;
}

void Mapper071::SaveState(saver& saver) {
	saver << prgBanks[0];
	saver << prgBanks[1];
}

void Mapper071::LoadState(saver& saver) {
	saver >> prgBanks[0];
	saver >> prgBanks[1];
}

}
