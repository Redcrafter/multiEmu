#include "Mapper071.h"

namespace Nes {

Mapper071::Mapper071(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr) : Mapper(prg, chr) {}

int Mapper071::cpuRead(uint16_t addr, uint8_t& data) {
	if(addr >= 0x8000) {
		data = prg[((addr & 0x3FFF) | (prgBanks[(addr >> 14) & 1] * 0x4000)) & prgMask];
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

uint8_t Mapper071::ppuRead(uint16_t addr, bool readOnly) {
	if(addr < 0x2000) {
		if(chr.empty())
			return chrRam[addr];
		return chr[addr];
	} else { // 0x2000 - 0x3EFF
		return vram[mapMirrorAddress(mirror, addr)];
	}
}

void Mapper071::SaveState(nlohmann::json& saver) const {
	saver["prgBanks[0]"] = prgBanks[0];
	saver["prgBanks[1]"] = prgBanks[1];
}

void Mapper071::LoadState(const nlohmann::json& saver) {
	prgBanks[0] = saver["prgBanks[0]"];
	prgBanks[1] = saver["prgBanks[1]"];
}

}
