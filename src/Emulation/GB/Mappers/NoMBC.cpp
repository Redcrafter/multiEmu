#include "NoMBC.h"

namespace Gameboy {

/*NoMBC::NoMBC(const std::vector<uint8_t>& rom) : rom(rom) {
    assert(rom.size() == 0x8000);
}*/

uint8_t NoMBC::CpuRead(uint16_t addr) const {
	if(addr < 0x8000) {
		return rom[addr];
	}

	if(hasRam) {
		return ram[addr & ramMask];
	}

	return 0;
}

void NoMBC::CpuWrite(uint16_t addr, uint8_t val) {
	if(0x8000 <= addr && hasRam) {
		ram[addr & ramMask] = val;
	}
}

}
