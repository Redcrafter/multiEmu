#include "Mapper000.h"

namespace Nes {

Mapper000::Mapper000(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr) : Mapper(prg, chr) {
	if(this->prg.size() > 0x8000) {
		throw std::invalid_argument("Invalid prg size");
	}
	if(this->chr.size() > 0x2000 || this->chr.empty()) {
		throw std::invalid_argument("Invalid chr size");
	}
}

int Mapper000::cpuRead(uint16_t addr, uint8_t& data) {
	if(addr >= 0x8000) {
		data = prg[addr & prgMask];
		return true;
	}
	return false;
}

uint8_t Mapper000::ppuRead(uint16_t addr, bool readOnly) {
	if(addr < 0x2000) {
		return chr[addr];
	} else {
		// 0x2000 - 0x3EFF
		return vram[mapMirrorAddress(mirror, addr)];
	}
}

void Mapper000::SaveState(nlohmann::json& saver) const {
	saver["vram"] = vram;
	saver["chrRam"] = chrRam;
	saver["Irq"] = Irq;
}

void Mapper000::LoadState(const nlohmann::json& saver) {
	vram = saver["vram"];
	chrRam = saver["chrRam"];
	Irq = saver["Irq"];
}

}
