#include "Gameboy.h"

#include <cstring>

#include "../../Input.h"

namespace Gameboy {

void Gameboy::Reset() {
	bool useBoot = false;

	ramBank = 1;

	std::memset(ram, 0, sizeof(ram));
	std::memset(hram, 0, sizeof(hram));

	if(useBoot) {
		inBios = true;

		InterruptFlag = 0;
	} else {
		inBios = false;

		cpu.Reset(false);

		// FF00 - FF0F
		JoyPadSelect = false;

		SB = 0;
		SC = 0x7E;
		DIV = 0x1838;
		TIMA = 0x00;
		TMA = 0x00;
		TAC = 0xF8;
		InterruptFlag = 0xE1;

		// FF10 - FF3F
		apu.reset();
		// FF40 - FF4B
		ppu.Reset();

		InterruptEnable = 0;
	}
}

uint8_t Gameboy::CpuRead(uint16_t addr) const {
	// printf("[0x%04X]\n", addr);

	switch(addr & 0xF000) {
		case 0x0000:
#if false
			if(inBios) {
				if(gbc) {
                    if(addr < sizeof(cgb_bios)) {
                        return cgb_bios[addr];
                    }
                } else {
                    if(addr < sizeof(dmg_boot)) {
                        return dmg_boot[addr];
                    }
                }
			}
#endif
		case 0x1000:
		case 0x2000:
		case 0x3000: // 0000-3FFF    16KB ROM Bank 00     (in cartridge, fixed at bank 00)
			return mbc->Bank0(addr);
		case 0x4000:
		case 0x5000:
		case 0x6000:
		case 0x7000: // 4000-7FFF    16KB ROM Bank 01..NN (in cartridge, switchable bank number)
			return mbc->CpuRead(addr);
		case 0x8000:
		case 0x9000: // 8000-9FFF    8KB Video RAM (VRAM) (switchable bank 0-1 in CGB Mode)
			if(ppu.STAT.modeFlag != 3) {
				// if not Transferring Data to LCD Driver
				return ppu.VRAM[ppu.vramBank][addr & 0x1FFF];
			} else {
				return 0xFF;
			}
		case 0xA000:
		case 0xB000: // A000-BFFF    8KB External RAM     (in cartridge, switchable bank, if any)
			return mbc->CpuRead(addr);
		case 0xC000: // C000-CFFF    4KB Work RAM Bank 0 (WRAM)
			return ram[0][addr & 0xFFF];
		case 0xD000: // D000-DFFF    4KB Work RAM Bank 1 (WRAM)  (switchable bank 1-7 in CGB Mode)
			return ram[ramBank][addr & 0xFFF];
		case 0xE000: // E000-EFFF    Same as C000-CFFF (ECHO)    (typically not used)
			return ram[0][addr & 0xFFF];
			// case 0xF000: // Fall through
	}

	if(addr <= 0xFDFF) {
		// F000-FDFF    Same as D000-DDFF (ECHO)    (typically not used)
		return ram[ramBank][addr & 0xFFF];
	} else if(addr <= 0xFE9F) {
		// Sprite Attribute Table (OAM)
		return (ppu.STAT.modeFlag == 0 || ppu.STAT.modeFlag == 1 || !ppu.Control.lcdEnable) ? ppu.OAM[addr & 0xFF] : 0xFF;
	} else if(addr <= 0xFEFF) {
		// FEA0-FEFF Not Usable
		// TODO: implement hardware behaviour
		return 0xFF;
	} else if(addr <= 0xFF7F) {
		// I/O Ports
		switch(addr) {
			case 0xFF00: {
				uint8_t val = 0;
				if(JoyPadSelect == 0) {
					val |= (!Input::GetKey(0)) << 0; // right
					val |= (!Input::GetKey(1)) << 1; // left
					val |= (!Input::GetKey(2)) << 2; // up
					val |= (!Input::GetKey(3)) << 3; // down
				} else {
					val |= (!Input::GetKey(4)) << 0; // A
					val |= (!Input::GetKey(5)) << 1; // B
					val |= (!Input::GetKey(6)) << 2; // Select
					val |= (!Input::GetKey(7)) << 3; // Start
				}
				return val | 0xC0;
			}
			case 0xFF01: return SB;
			case 0xFF02: return SC;
			case 0xFF04: return DIV >> 8;
			case 0xFF05: return TIMA;
			case 0xFF06: return TMA;
			case 0xFF07: return TAC;
			case 0xFF0F: return InterruptFlag;

			case 0xFF10: case 0xFF11: case 0xFF12: case 0xFF13: case 0xFF14: // Sound Channel 1:
			case 0xFF16: case 0xFF17: case 0xFF18: case 0xFF19: // Sound Channel 2:
			case 0xFF1A: case 0xFF1B: case 0xFF1C: case 0xFF1D: case 0xFF1E: // Sound Channel 3:
			case 0xFF20: case 0xFF21: case 0xFF22: case 0xFF23: // Sound Channel 4:
			case 0xFF24: case 0xFF25: case 0xFF26: // Sound Control:
			case 0xFF30: case 0xFF31: case 0xFF32: case 0xFF33: case 0xFF34: case 0xFF35: case 0xFF36: case 0xFF37: // FF30-FF3F Wave Pattern RAM
			case 0xFF38: case 0xFF39: case 0xFF3A: case 0xFF3B: case 0xFF3C: case 0xFF3D: case 0xFF3E: case 0xFF3F:
				return apu.read(addr);
			case 0xFF40: return ppu.Control.reg;
			case 0xFF41: return ppu.STAT.reg | 0x80;
			case 0xFF42: return ppu.SCY;
			case 0xFF43: return ppu.SCX;
			case 0xFF44: return ppu.LY;
			case 0xFF45: return ppu.LYC;
			case 0xFF46: return 0xFF;
			case 0xFF47: return ppu.BGP;
			case 0xFF48: return ppu.OBP0;
			case 0xFF49: return ppu.OBP1;
			case 0xFF4A: return ppu.WY;
			case 0xFF4B: return ppu.WX;
			case 0xFF4F: if(gbc) return ppu.vramBank | 0xFE;
		}
		return 0xFF;
	} else if(addr <= 0xFFFE) { // FF80-FFFE High RAM
		return hram[addr & 0x7F];
	} else { // Interrupt Enable Register
		return InterruptEnable;
	}
}

void Gameboy::CpuWrite(uint16_t addr, uint8_t val) {
	// printf("[0x%04X] = %i\n", addr, val);

	switch(addr & 0xF000) {
		case 0x0000:
		case 0x1000:
		case 0x2000:
		case 0x3000: // 0000-3FFF    16KB ROM Bank 00     (in cartridge, fixed at bank 00)
		case 0x4000:
		case 0x5000:
		case 0x6000:
		case 0x7000: // 4000-7FFF    16KB ROM Bank 01..NN (in cartridge, switchable bank number)
		case 0xA000:
		case 0xB000: // A000-BFFF    8KB External RAM     (in cartridge, switchable bank, if any)
			mbc->CpuWrite(addr, val);
			break;
		case 0x8000:
		case 0x9000: // 8000-9FFF    8KB Video RAM (VRAM) (switchable bank 0-1 in CGB Mode)
					 // if(ppu.STAT.modeFlag != 3) {
			// if not Transferring Data to LCD Driver
			ppu.VRAM[ppu.vramBank][addr & 0x1FFF] = val;
			// }
			break;
		case 0xC000: // C000-CFFF    4KB Work RAM Bank 0 (WRAM)
			ram[0][addr & 0xFFF] = val;
			break;
		case 0xD000: // D000-DFFF    4KB Work RAM Bank 1 (WRAM)  (switchable bank 1-7 in CGB Mode)
			ram[ramBank][addr & 0xFFF] = val;
			break;
		case 0xE000: // E000-EFFF    Same as C000-CFFF (ECHO)    (typically not used)
			ram[0][addr & 0xFFF] = val;
			break;
		case 0xF000:
			if(addr <= 0xFDFF) {
				// F000-FDFF    Same as D000-DDFF (ECHO)    (typically not used)
				ram[ramBank][addr & 0xFFF] = val;
			} else if(addr <= 0xFE9F) {
				// FE00-FE9F Sprite Attribute Table (OAM)
				if(ppu.STAT.modeFlag == 0 || ppu.STAT.modeFlag == 1 || !ppu.Control.lcdEnable) {
					ppu.OAM[addr & 0xFF] = val;
				}
			} else if(addr <= 0xFEFF) {
				// FEA0-FEFF Not Usable
			} else if(addr <= 0xFF7F) {
				switch(addr) {
					case 0xFF00:
						if(!(val & 0x10)) {
							JoyPadSelect = 0;
						} else if(!(val & 0x20)) {
							JoyPadSelect = 1;
						}
						break;
					case 0xFF01: // SB
						// printf("%c", val);
						break;
					case 0xFF02: // SC
						// printf("SC %i\n", val);
						// serial bus ignore
						break;
					case 0xFF04: DIV = 0; break;
					case 0xFF05: TIMA = val; break;
					case 0xFF06: TMA = val; break;
					case 0xFF07: TAC = val; break;
					case 0xFF0F: InterruptFlag = val & 0x1F; break;
					case 0xFF10:
					case 0xFF11:
					case 0xFF12:
					case 0xFF13:
					case 0xFF14: // Sound Channel 1:
					case 0xFF16:
					case 0xFF17:
					case 0xFF18:
					case 0xFF19: // Sound Channel 2:
					case 0xFF1A:
					case 0xFF1B:
					case 0xFF1C:
					case 0xFF1D:
					case 0xFF1E: // Sound Channel 3:
					case 0xFF20:
					case 0xFF21:
					case 0xFF22:
					case 0xFF23: // Sound Channel 4:
					case 0xFF24:
					case 0xFF25:
					case 0xFF26: // Sound Control:
					case 0xFF30:
					case 0xFF31:
					case 0xFF32:
					case 0xFF33:
					case 0xFF34:
					case 0xFF35:
					case 0xFF36:
					case 0xFF37: // FF30-FF3F Wave Pattern RAM
					case 0xFF38:
					case 0xFF39:
					case 0xFF3A:
					case 0xFF3B:
					case 0xFF3C:
					case 0xFF3D:
					case 0xFF3E:
					case 0xFF3F:
						apu.write(addr, val);
						break;
					case 0xFF40: ppu.Control.reg = val; break;
					case 0xFF41: ppu.STAT.reg = val & 0xF0; break;
					case 0xFF42: ppu.SCY = val; break;
					case 0xFF43: ppu.SCX = val; break;
					case 0xFF44: ppu.LY = val; break;
					case 0xFF45: ppu.LYC = val; break;
					case 0xFF46: // DMA
						// TODO: do actual dma instead of this
						for(int i = 0; i < 0xA0; i++) {
							ppu.OAM[i] = CpuRead((val << 8) | i);
						}
						break;
					case 0xFF47: ppu.BGP = val; break;
					case 0xFF48: ppu.OBP0 = val; break;
					case 0xFF49: ppu.OBP1 = val; break;
					case 0xFF4A: ppu.WY = val; break;
					case 0xFF4B: ppu.WX = val; break;
					case 0xFF4F:
						if(gbc) {
							ppu.vramBank = val & 1;
						}
						break;
					case 0xFF50:
						inBios = !val;
						// cpu.startLog();
						break;
					case 0xFF70:
						ramBank = val & 7;
						break;
					default: // __debugbreak();
						break;
				}
			} else if(addr <= 0xFFFE) {
				// FF80-FFFE High RAM
				hram[addr & 0x7F] = val;
			} else {
				InterruptEnable = val & 0x1F;
			}
			break;
	}
}

void Gameboy::Clock() {
	cpu.Clock();
	ppu.Clock();

	apu.clock();

	const uint16_t rates[] = { 1 << 9, 1 << 3, 1 << 5, 1 << 7 };
	if((TAC & 4) && DIV & rates[TAC & 3]) {
		if(TIMA == 0xFF) {
			TIMA = TMA;
			Interrupt(Interrupt::Timer);
		} else {
			TIMA++;
		}
	}

	DIV += 4;
}

}
