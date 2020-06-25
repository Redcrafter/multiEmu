#include "NsfMapper.h"
#include <cstring>

//NSF ROM and general approaches are heavily derived from BizHawk. the general ideas:
//1. Have a hardcoded NSF driver rom loaded to 0x3800
//2. Have fake registers at $3FFx for the NSF driver to use
//3. These addresses are chosen because no known NSF could possibly use them for anything.
//4. Patch the PRG with our own IRQ vectors when the NSF play and init routines aren't running. 
//   That way we can use NMI for overall control and cause our code to be the NMI handler without breaking the NSF data by corrupting the last few bytes

const uint16_t NMI_VECTOR = 0x3800;
const uint16_t RESET_VECTOR = 0x3820;
// 
NsfMapper::NsfMapper(const std::string& path) : Mapper({}, {}), nsf(path) {
	NSFROM[0x12] = nsf.initAddress;
	NSFROM[0x13] = nsf.initAddress >> 8;
	NSFROM[0x19] = nsf.playAddress;
	NSFROM[0x1A] = nsf.playAddress >> 8;

	BankSwitched = false;
	for(int i = 0; i < 8; i++) {
		int bank = nsf.bankInit[i];

		if(bank * 4096 >= nsf.length)
			bank = 0;

		InitBankSwitches[i] = bank;
		prg_banks_4k[i] = bank;
		if(bank != 0)
			BankSwitched = true;
	}

	if(!BankSwitched) {
		int load_start = nsf.loadAddress - 0x8000;
		int load_size = nsf.length;

		std::memset(FakePRG, 0, sizeof(FakePRG));
		std::memcpy(FakePRG + load_start, nsf.rom, load_size);
	}
}

int NsfMapper::cpuRead(uint16_t addr, uint8_t& data) {
	if(addr >= NMI_VECTOR && addr - NMI_VECTOR <= sizeof(NSFROM)) {
		data = NSFROM[addr - NMI_VECTOR];
		return true;
	}

	switch(addr) {
		case 0x3FF0:
			data = InitPending;
			InitPending = false;
			return true;
		case 0x3FF1:
			// printf("Ehhh");
			data = 0;
			return true;
		case 0x3FF2:
			data = 0; //always return NTSC for now
			return true;
		case 0x3FF3:
			Patch_Vectors = false;
			return true;
		case 0x3FF4:
			Patch_Vectors = true;
			return true;
	}

	if(addr < 0x8000) {
		return false;
	}

	if(Patch_Vectors) {
		switch(addr) {
			case 0xFFFA: data = NMI_VECTOR & 0xFF; break;
			case 0xFFFB: data = (NMI_VECTOR >> 8) & 0xFF; break;
			case 0xFFFC: data = RESET_VECTOR & 0xFF; break;
			case 0xFFFD: data = (RESET_VECTOR >> 8) & 0xFF; break;
			default: data = 0; break;
		}
	} else {
		addr -= 0x8000;
		if(BankSwitched) {
			data = nsf.rom[(addr & 0xFFF) | (prg_banks_4k[(addr >> 12) & 0b111] << 12)];;
		} else {
			data = FakePRG[addr];
		}
	}

	return true;
}

bool NsfMapper::cpuWrite(uint16_t addr, uint8_t data) {
	switch(addr) {
		case 0x5FF6:
		case 0x5FF7:
			break;
		case 0x5FF8:
		case 0x5FF9:
		case 0x5FFA:
		case 0x5FFB:
		case 0x5FFC:
		case 0x5FFD:
		case 0x5FFE:
		case 0x5FFF:
			if(!BankSwitched) {
				break;
			}
			prg_banks_4k[addr - 0x5FF8] = data;
			return true;
	}

	return false;
}

bool NsfMapper::ppuRead(uint16_t addr, uint8_t& data, bool readOnly) {
	return false;
}

bool NsfMapper::ppuWrite(uint16_t addr, uint8_t data) {
	return false;
}

void NsfMapper::SaveState(saver& saver) { }
void NsfMapper::LoadState(saver& saver) { }
