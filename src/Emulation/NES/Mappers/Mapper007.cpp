#include "Mapper007.h"
#include "../Cartridge.h"

namespace Nes {

Mapper007::Mapper007(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr) : Mapper(prg, chr) {
	if(chr.size() > 0x2000) {
		throw std::invalid_argument("Invalid chr size");
	}
}

int Mapper007::cpuRead(uint16_t addr, uint8_t& data) {
	if(addr >= 0x8000) {
		data = prg[((addr & 0x7FFF) | (prgBank * 0x8000)) & prgMask];
		return true;
	}
	return false;
}

bool Mapper007::cpuWrite(uint16_t addr, uint8_t data) {
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

bool Mapper007::ppuRead(uint16_t addr, uint8_t& data, bool readOnly) {
	if(addr < 0x2000 && !chr.empty()) {
		data = chr[addr];
		return true;
	}
	return false;
}

void Mapper007::SaveState(saver& saver) {
	saver << prgBank;
}

void Mapper007::LoadState(saver& saver) {
	saver >> prgBank;
}

}
