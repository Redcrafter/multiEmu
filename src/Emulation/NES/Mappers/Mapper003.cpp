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

uint8_t Mapper003::ppuRead(uint16_t addr, bool readOnly) {
	if(addr < 0x2000) {
		return chr[((addr & 0x1FFF) | (selectedBank * 0x2000)) & chrMask];
	} else { // 0x2000 - 0x3EFF
		return vram[mapMirrorAddress(mirror, addr)];
	}
}

void Mapper003::SaveState(nlohmann::json& saver) const {
	saver["selectedBank"] = selectedBank;
}

void Mapper003::LoadState(const nlohmann::json& saver) {
	selectedBank = saver["selectedBank"];
}

}
