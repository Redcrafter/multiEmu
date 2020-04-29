#include "Mapper001.h"
#include "../Cartridge.h"

Mapper001::Mapper001(uint8_t prgBanks, uint8_t chrBanks) : Mapper(prgBanks, chrBanks) {
	prgBankOffset[0] = 0;
	prgBankOffset[1] = (prgBanks - 1) * 0x4000;

	chrBankOffset[0] = 0;
	chrBankOffset[1] = 0x1000;
}

int Mapper001::cpuMapRead(uint16_t addr, uint32_t& mapped) {
	lastWrite = false;
	if(addr < 0x8000) {
		if(ramEnable && addr >= 0x6000) {
			mapped = prgRam[addr & 0x1FFF];
			return 2;
		}
		return false;
	}

	mapped = (addr & 0x3FFF) | prgBankOffset[(addr >> 14) & 1];
	return true;
}

bool Mapper001::cpuMapWrite(uint16_t addr, uint8_t data) {
	if(addr < 0x8000) {
		if(addr >= 0x6000) {
			prgRam[addr & 0x1FFF] = data;
			return true;
		}
		return false;
	}
	if(lastWrite) {
		return false;
	}

	if(data & 0x80) {
		shiftRegister = 0b100000;
		Control |= 0x0C;
		lastWrite = true;

		return false;
	}

	shiftRegister >>= 1;
	shiftRegister |= (data & 1) << 5;

	if(shiftRegister & 1) {
		shiftRegister >>= 1;

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
				ramEnable = shiftRegister & 0x10;
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
			chrBankOffset[0] = (chrBank0) * 0x1000;
			chrBankOffset[1] = (chrBank1) * 0x1000;
		} else {
			chrBankOffset[0] = (chrBank0 & ~1) * 0x1000;
			chrBankOffset[1] = (chrBank0 & ~1) * 0x1000 + 0x1000;
		}

		shiftRegister = 0b100000;
	}

	return false;
}

bool Mapper001::ppuMapRead(uint16_t addr, uint32_t& mapped, bool readOnly) {
	if(addr < 0x2000) {
		mapped = (addr & 0x0FFF) + chrBankOffset[(addr >> 12) & 1];
		return true;
	}
	return false;
}

bool Mapper001::ppuMapWrite(uint16_t addr, uint8_t data) {
	return false;
}

void Mapper001::SaveState(saver& saver) {
	saver << Control;
	saver << shiftRegister;
	saver << chrBank0;
	saver << chrBank1;
	saver << prgBank;

	saver << prgBankOffset[0];
	saver << prgBankOffset[1];

	saver << chrBankOffset[0];
	saver << chrBankOffset[1];

	saver.Write((char*)prgRam, sizeof(prgRam));
}

void Mapper001::LoadState(saver& saver) {
	// uint8_t temp = 0;
	// saver >> temp;
	saver >> Control;
	saver >> shiftRegister;
	saver >> chrBank0;
	saver >> chrBank1;
	saver >> prgBank;

	saver >> prgBankOffset[0];
	saver >> prgBankOffset[1];

	saver >> chrBankOffset[0];
	saver >> chrBankOffset[1];

	saver.Read((char*)prgRam, sizeof(prgRam));
}
