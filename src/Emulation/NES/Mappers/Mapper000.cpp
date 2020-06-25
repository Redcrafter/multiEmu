#include "Mapper000.h"

Mapper000::Mapper000(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr) : Mapper(prg, chr) {
	if(this->prg.size() > 0x8000) {
		throw std::invalid_argument("Invalid prg size");
	}
	if(this->chr.size() > 0x2000) {
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

bool Mapper000::ppuRead(uint16_t addr, uint8_t& data, bool readOnly) {
	if(addr < 0x2000 && !chr.empty()) {
		data = chr[addr];
		return true;
	}
	return false;
}
