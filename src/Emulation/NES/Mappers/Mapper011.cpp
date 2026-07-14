#include "Mapper011.h"

namespace Nes {

Mapper011::Mapper011(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr) : Mapper(prg, chr) {}

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

uint8_t Mapper011::ppuRead(uint16_t addr, bool readOnly) {
	if(addr < 0x2000) {
		return chr[((addr & 0x1FFF) | (chrBank * 0x2000)) & chrMask];
	} else {
		// 0x2000 - 0x3EFF
		return vram[mapMirrorAddress(mirror, addr)];
	}
}

void Mapper011::SaveState(nlohmann::json& saver) const {
	saver["prgBank"] = prgBank;
	saver["chrBank"] = chrBank;
}

void Mapper011::LoadState(const nlohmann::json& saver) {
	prgBank = saver["prgBank"];
	chrBank = saver["chrBank"];
}

}
