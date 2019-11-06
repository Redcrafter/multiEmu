#include "Mapper001.h"
#include "../Emulation/Cartridge.h"

Mapper001::Mapper001(uint8_t prgBanks, uint8_t chrBanks) : Mapper(prgBanks, chrBanks) {
	prgBankOffset[0] = 0;
	prgBankOffset[1] = (prgBanks - 1) * 0x4000;

	chrBankOffset[0] = 0;
	chrBankOffset[1] = 0x1000;
}

int Mapper001::cpuMapRead(uint16_t addr, uint32_t& mapped) {
	if(addr < 0x8000) {
		return false; // TODO: ram?
	}

	mapped = (addr & 0x3FFF) | prgBankOffset[(addr >> 14) & 1];
	return true;
}

bool Mapper001::cpuMapWrite(uint16_t addr, uint8_t data) {
	if(addr < 0x8000) {
		return false;
	}

	if(data & 0x80) {
		shiftRegister = 0;
		writeState = 1;
	}

	if(writeState) {
		shiftRegister >>= 1;
		shiftRegister |= (data & 1) << 7;

		writeState++;
	}

	if(writeState == 6) {
		writeState = 0;
		shiftRegister >>= 3;

		switch((addr >> 13) & 3) {
			case 0:
				switch(shiftRegister & 3) {
					case 0:
						mirror = MirrorMode::OnescreenLo;
						break;
					case 1:
						mirror = MirrorMode::OnescreenHi;
						break;
					case 2:
						mirror = MirrorMode::Vertical;
						break;
					case 3:
						mirror = MirrorMode::Horizontal;
						break;
				}

				Control = shiftRegister;
				break;
			case 1:
				chrBank0 = shiftRegister & 0x1F;
				break;
			case 2:
				chrBank1 = shiftRegister & 0x1F;
				break;
			case 3:
				prgBank = shiftRegister & 0xF;
				// Todo: ram
				break;
		}

		switch((Control >> 2) & 3) {
			case 0:
			case 1:
				prgBankOffset[0] = (prgBank & ~1) * 0x4000;
				prgBankOffset[1] = (prgBank & ~1) * 0x4000 + 0x4000;
				break;
			case 2:
				prgBankOffset[0] = 0;
				prgBankOffset[1] = prgBank * 0x4000;
				break;
			case 3:
				prgBankOffset[0] = prgBank * 0x4000;
				prgBankOffset[1] = (prgBanks - 1) * 0x4000;
				break;
		}

		if(Control & 0x10) {
			chrBankOffset[0] = (chrBank0 & ~1) * 0x1000;
			chrBankOffset[1] = (chrBank0 & ~1) * 0x1000 + 0x1000;
		} else {
			chrBankOffset[0] = (chrBank0) * 0x1000;
			chrBankOffset[1] = (chrBank1) * 0x1000;
		}

		shiftRegister = 0;
	}

	return false;
}

bool Mapper001::ppuMapRead(uint16_t addr, uint32_t& mapped) {
	if(addr < 0x2000) {
		mapped = addr & 0x0FFF + chrBankOffset[(addr >> 12) & 1];
		return true;
	}
	return false;
}

bool Mapper001::ppuMapWrite(uint16_t addr, uint8_t data) {
	return false;
}

void Mapper001::SaveState(saver& saver) {
	saver << writeState;
	saver << Control;
	saver << shiftRegister;
	saver << chrBank0;
	saver << chrBank1;
	saver << prgBank;

	saver << prgBankOffset[0];
	saver << prgBankOffset[1];

	saver << chrBankOffset[0];
	saver << chrBankOffset[1];
}

void Mapper001::LoadState(saver& saver) {
	saver >> writeState;
	saver >> Control;
	saver >> shiftRegister;
	saver >> chrBank0;
	saver >> chrBank1;
	saver >> prgBank;

	saver >> prgBankOffset[0];
	saver >> prgBankOffset[1];

	saver >> chrBankOffset[0];
	saver >> chrBankOffset[1];
}
