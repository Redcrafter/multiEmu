#include "Gameboy.h"

#include <cstring>

#include "../../Input.h"

static Input::Mapper inputMapper ("GB", {
	{ "Right",  0, { GLFW_KEY_RIGHT, 0 } },
	{ "Left",   1, { GLFW_KEY_LEFT,  0 } },
	{ "Up",     2, { GLFW_KEY_UP,    0 } },
	{ "Down",   3, { GLFW_KEY_DOWN,  0 } },
	{ "A",      4, { GLFW_KEY_A,     0 } },
	{ "B",      5, { GLFW_KEY_B,     0 } },
	{ "Select", 6, { GLFW_KEY_ENTER, 0 } },
	{ "Start",  7, { GLFW_KEY_S,     0 } },
});

namespace Gameboy {

#define GB_TIMA_RUNNING 0
#define GB_TIMA_RELOADING 1
#define GB_TIMA_RELOADED 2

void Gameboy::Reset(Mode mode) {
	// bool useBoot = false;

	std::memset(&ram, 0, sizeof(ram));
	hram.fill(0);

	inBios = false;

	cpu.Reset(mode);

	JoyPadSelect = 0;	 // FF00
	SB = 0;				 // FF01
	SC = 0;				 // FF02
	TIMA = 0;			 // FF05
	TMA = 0;			 // FF06
	TAC = 0;			 // FF07
	InterruptFlag = 1;	 // FF0F
	apu.reset();		 // FF10 - FF3F
	ppu.Reset();		 // FF40 - FF4B
	speed = 0;			 // FF4D
	vramBank = 0;		 // FF4F
	HDMA1 = 0xFF;		 // FF51
	HDMA2 = 0xFF;		 // FF52
	HDMA3 = 0xFF;		 // FF53
	HDMA4 = 0xFF;		 // FF54
	RP = 0xFF;			 // FF56
	BGPI = 0xFF;		 // FF68
	OBPI = 0xFF;		 // FF6A
	ramBank = 7;		 // FF70
	FF72 = 0xFF;		 // FF72
	FF73 = 0xFF;		 // FF73
	FF74 = 0xFF;		 // FF74
	FF75 = 0xFF;		 // FF75
	InterruptEnable = 0; // FFFF

	lastTimer = false;
	timaState = GB_TIMA_RUNNING;
	pendingCycles = 0;
	cyclesPassed = 0;

	dmaReg = 0xFF;
	dmaDest = 0xA0;

	auto cgbFlag = CpuRead(0x0143);

	gbc = apu.gbc = false;
	switch(mode) {
		case Mode::DMG0:
			DIV = 0x1800; // FF04
			break;
		case Mode::DMG:
		case Mode::MGB:
			DIV = 0xABD4; // FF04
			break;
		case Mode::SGB:
		case Mode::SGB2:
			CpuWrite(0xFF26, 0xF0);
			break;
		case Mode::CGB:
		case Mode::AGB:
			SC = 3;		  // FF02
			DIV = 0xA344; // FF04
			dmaReg = 0;

			FF72 = 0;
			FF73 = 0;
			FF74 = 0;
			FF75 = 0;

			speed = 0x7E;

			RP = 0xC1;
			BGPI = 0;
			OBPI = 0;

			if(cgbFlag == 0x80 || cgbFlag == 0xC0) {
				gbc = apu.gbc = true;
			} else { // DMG mode
			}
			break;
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
			return mbc->Read0(addr);
		case 0x4000:
		case 0x5000:
		case 0x6000:
		case 0x7000: // 4000-7FFF    16KB ROM Bank 01..NN (in cartridge, switchable bank number)
			return mbc->Read4(addr);
		case 0x8000:
		case 0x9000: // 8000-9FFF    8KB Video RAM (VRAM) (switchable bank 0-1 in CGB Mode)
			if(ppu.STAT.modeFlag != 3 || !ppu.Control.lcdEnable) {
				// if not Transferring Data to LCD Driver
				return ppu.VRAM[vramBank][addr & 0x1FFF];
			} else {
				return 0xFF;
			}
		case 0xA000:
		case 0xB000: // A000-BFFF    8KB External RAM     (in cartridge, switchable bank, if any)
			return mbc->ReadA(addr);
		case 0xC000: // C000-CFFF    4KB Work RAM Bank 0 (WRAM)
			return ram[0][addr & 0xFFF];
		case 0xD000: // D000-DFFF    4KB Work RAM Bank 1 (WRAM)  (switchable bank 1-7 in CGB Mode)
			return ram[std::max(ramBank & 7, 1)][addr & 0xFFF];
		case 0xE000: // E000-EFFF    Same as C000-CFFF (ECHO)    (typically not used)
			return ram[0][addr & 0xFFF];
			// case 0xF000: // Fall through
	}

	if(addr <= 0xFDFF) {
		// F000-FDFF    Same as D000-DDFF (ECHO)    (typically not used)
		return ram[std::max(ramBank & 7, 1)][addr & 0xFFF];
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
				if(JoyPadSelect == 1) {
					val |= (!inputMapper.GetKey(0)) << 0; // right
					val |= (!inputMapper.GetKey(1)) << 1; // left
					val |= (!inputMapper.GetKey(2)) << 2; // up
					val |= (!inputMapper.GetKey(3)) << 3; // down
					val |= 0x20;
				} else if(JoyPadSelect == 2) {
					val |= (!inputMapper.GetKey(4)) << 0; // A
					val |= (!inputMapper.GetKey(5)) << 1; // B
					val |= (!inputMapper.GetKey(6)) << 2; // Select
					val |= (!inputMapper.GetKey(7)) << 3; // Start
					val |= 0x10;
				}
				return val | 0xC0;
			}
			case 0xFF01: return SB;
			case 0xFF02: return SC | (gbc ? 0x7C : 0x7E);
			case 0xFF04: return DIV >> 8;
			case 0xFF05: return timaState == GB_TIMA_RELOADING ? 0 : TIMA;
			case 0xFF06: return TMA;
			case 0xFF07: return TAC | 0xF8;
			case 0xFF0F: return InterruptFlag | 0xE0;

			case 0xFF10: case 0xFF11: case 0xFF12: case 0xFF13: case 0xFF14: // Sound Channel 1:
			case 0xFF16: case 0xFF17: case 0xFF18: case 0xFF19: // Sound Channel 2:
			case 0xFF1A: case 0xFF1B: case 0xFF1C: case 0xFF1D: case 0xFF1E: // Sound Channel 3:
			case 0xFF20: case 0xFF21: case 0xFF22: case 0xFF23: // Sound Channel 4:
			case 0xFF24: case 0xFF25: case 0xFF26: // Sound Control:
			case 0xFF30: case 0xFF31: case 0xFF32: case 0xFF33: case 0xFF34: case 0xFF35: case 0xFF36: case 0xFF37: // FF30-FF3F Wave Pattern RAM
			case 0xFF38: case 0xFF39: case 0xFF3A: case 0xFF3B: case 0xFF3C: case 0xFF3D: case 0xFF3E: case 0xFF3F:
				return apu.read(addr);
			case 0xFF40: return ppu.Control.reg;
			case 0xFF41: return ppu.STAT.reg;
			case 0xFF42: return ppu.SCY;
			case 0xFF43: return ppu.SCX;
			case 0xFF44: return ppu.LY;
			case 0xFF45: return ppu.LYC;
			case 0xFF46: return dmaReg;
			case 0xFF47: return ppu.BGP;
			case 0xFF48: return ppu.OBP0;
			case 0xFF49: return ppu.OBP1;
			case 0xFF4A: return ppu.WY;
			case 0xFF4B: return ppu.WX;

			case 0xFF4D: return gbc ? speed : 0xFF;
			case 0xFF4F: return gbc ? vramBank | 0xFE : 0xFF;
			case 0xFF56: return RP;
			case 0xFF68: return BGPI | 0x40;
			case 0xFF69: return gbc ? ppu.gbcBGP[BGPI & 0x3F] : 0xFF;
			case 0xFF6A: return OBPI | 0x40;
			case 0xFF6B: return gbc ? ppu.gbcOBP[OBPI & 0x3F] : 0xFF;
			case 0xFF6C: return gbc ? ppu.OPRI | 0xFE : 0xFF;
			case 0xFF70: return ramBank | 0xF8;
			case 0xFF72: return FF72;
			case 0xFF73: return FF73;
			case 0xFF74: return FF74;
			case 0xFF75: return FF75 | 0x8F;
			case 0xFF76: return apu.FF76();
			case 0xFF77: return apu.FF77();
			// case 0xFF77: // TODO: PCM amplitudes 3 & 4
			default: return 0xFF;
		}
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
			mbc->Write0(addr, val);
			break;
		case 0x4000:
		case 0x5000:
		case 0x6000:
		case 0x7000: // 4000-7FFF    16KB ROM Bank 01..NN (in cartridge, switchable bank number)
			mbc->Write4(addr, val);
			break;
		case 0x8000:
		case 0x9000: // 8000-9FFF    8KB Video RAM (VRAM) (switchable bank 0-1 in CGB Mode)
			if(ppu.STAT.modeFlag != 3 || !ppu.Control.lcdEnable) {
				// if not Transferring Data to LCD Driver
				ppu.VRAM[vramBank][addr & 0x1FFF] = val;
			}
			break;
		case 0xA000:
		case 0xB000: // A000-BFFF    8KB External RAM     (in cartridge, switchable bank, if any)
			mbc->WriteA(addr, val);
			break;
		case 0xC000: // C000-CFFF    4KB Work RAM Bank 0 (WRAM)
			ram[0][addr & 0xFFF] = val;
			break;
		case 0xD000: // D000-DFFF    4KB Work RAM Bank 1 (WRAM)  (switchable bank 1-7 in CGB Mode)
			ram[std::max(ramBank & 7, 1)][addr & 0xFFF] = val;
			break;
		case 0xE000: // E000-EFFF    Same as C000-CFFF (ECHO)    (typically not used)
			ram[0][addr & 0xFFF] = val;
			break;
		case 0xF000:
			if(addr <= 0xFDFF) {
				// F000-FDFF    Same as D000-DDFF (ECHO)    (typically not used)
				ram[std::max(ramBank & 7, 1)][addr & 0xFFF] = val;
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
							JoyPadSelect = 1;
						} else if(!(val & 0x20)) {
							JoyPadSelect = 2;
						} else {
							JoyPadSelect = 0;
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
					case 0xFF05:
						if(timaState != GB_TIMA_RELOADED) TIMA = val;
						break;
					case 0xFF06:
						TMA = val;
						if(timaState != GB_TIMA_RUNNING) TIMA = val;
						break;
					case 0xFF07: TAC = val; break;
					case 0xFF0F: InterruptFlag = val & 0x1F; break;
					case 0xFF10: case 0xFF11: case 0xFF12: case 0xFF13: case 0xFF14: // Sound Channel 1:
					case 0xFF16: case 0xFF17: case 0xFF18: case 0xFF19: // Sound Channel 2:
					case 0xFF1A: case 0xFF1B: case 0xFF1C: case 0xFF1D: case 0xFF1E: // Sound Channel 3:
					case 0xFF20: case 0xFF21: case 0xFF22: case 0xFF23: // Sound Channel 4:
					case 0xFF24: case 0xFF25: case 0xFF26: // Sound Control:
					case 0xFF30: case 0xFF31: case 0xFF32: case 0xFF33: case 0xFF34: case 0xFF35: case 0xFF36: case 0xFF37: // FF30-FF3F Wave Pattern RAM
					case 0xFF38: case 0xFF39: case 0xFF3A: case 0xFF3B: case 0xFF3C: case 0xFF3D: case 0xFF3E: case 0xFF3F:
						apu.write(addr, val);
						break;
					case 0xFF40:
						ppu.Control.reg = val;
						if(!ppu.Control.lcdEnable) {
							ppu.STAT.modeFlag = 0;
							ppu.LY = 0;
							ppu.LX = 4;
						}
						break;
					case 0xFF41: ppu.STAT.reg = (val & 0x78) | (ppu.STAT.reg & 0x87); break;
					case 0xFF42: ppu.SCY = val; break;
					case 0xFF43: ppu.SCX = val; break;
					case 0xFF44: ppu.LY = val; break;
					case 0xFF45: ppu.LYC = val; break;
					case 0xFF46: // DMA
						dmaCycles = -7;
						dmaDest = 0;
						dmaSrc = val << 8;
						dmaReg = val;
						break;
					case 0xFF47: ppu.BGP = val; break;
					case 0xFF48: ppu.OBP0 = val; break;
					case 0xFF49: ppu.OBP1 = val; break;
					case 0xFF4A: ppu.WY = val; break;
					case 0xFF4B: ppu.WX = val; break;
					case 0xFF4D: if(gbc) speed = (speed & 0x80) | (val & 0x7F); break;
					case 0xFF4F: if(gbc) vramBank = val & 1; break;
					case 0xFF50: inBios = !val; break;
					case 0xFF51: if(gbc) HDMA1 = val; break;
					case 0xFF52: if(gbc) HDMA2 = val; break;
					case 0xFF53: if(gbc) HDMA3 = val; break;
					case 0xFF54: if(gbc) HDMA4 = val; break;
					case 0xFF55:
						if(gbc) {
							auto cpu = ((HDMA1 << 8) | HDMA2) & 0xFFF0;
							auto vram = ((HDMA3 << 4) | (HDMA4 >> 4));
							auto len = ((val & 0x7F) + 1) * 0x10;
							for(int i = 0; i < len; i++) {
								ppu.VRAM[vramBank][vram & 0x1FF] = CpuRead(cpu);
								cpu++;
								vram++;
							}
						}
						break;
					case 0xFF56: if(gbc) RP = val & 0xC1; break;
					case 0xFF68: if(gbc) BGPI = val; break;
					case 0xFF69:
						if(gbc) {
							ppu.gbcBGP[BGPI & 0x3F] = val;
							if(BGPI & 0x80) BGPI = (BGPI & 0x80) | ((BGPI & 0x3F) + 1);
						}
						break;
					case 0xFF6A: if(gbc) OBPI = val; break;
					case 0xFF6B:
						if(gbc) {
							ppu.gbcOBP[OBPI & 0x3F] = val;
							if(OBPI & 0x80) OBPI = (OBPI & 0x80) | ((OBPI & 0x3F) + 1);
						}
						break;
					case 0xFF6C: if(gbc) ppu.OPRI = val & 1; break;
					case 0xFF70: if(gbc) ramBank = val & 7; break;
					case 0xFF72: if(gbc) FF72 = val; break;
					case 0xFF73: if(gbc) FF73 = val; break;
					case 0xFF74: if(gbc) FF74 = val; break;
					case 0xFF75: if(gbc) FF75 = val & 0x70; break;
					default: break;
				}
			} else if(addr <= 0xFFFE) {
				// FF80-FFFE High RAM
				hram[addr & 0x7F] = val;
			} else {
				InterruptEnable = val;
			}
			break;
	}
}

void Gameboy::Clock() {
	cpu.Step();
}

void Gameboy::Advance() {
	if(pendingCycles == 0) return;

	assert(pendingCycles % 4 == 0);

	for(size_t i = 0; i < pendingCycles / 4; i++) {
		clockTimer();
	}
	clockDma(pendingCycles);

	// cpu, timer and dma run at double speed
	int count = (speed & 0x80) ? pendingCycles / 2 : pendingCycles;
	ppu.Clock(count);
	apu.clock(count);

	cyclesPassed += count;
	pendingCycles = 0;
}

void Gameboy::TriggerOamBug(uint16_t address) {
	if(gbc) return;

    // todo: https://gbdev.io/pandocs/OAM_Corruption_Bug.html

	/*if(address >= 0xFE00 && address < 0xFF00) {
	}*/
}

void Gameboy::SaveState(nlohmann::json& saver) const {
	cpu.SaveState(saver["cpu"]);
	ppu.SaveState(saver["ppu"]);
	apu.SaveState(saver["apu"]);
	mbc->SaveState(saver["mbc"]);

	saver["DIV"] = DIV;
	saver["ram"] = ram;
	saver["hram"] = hram;
	saver["ramBank"] = ramBank;
	saver["InterruptEnable"] = InterruptEnable;
	saver["InterruptFlag"] = InterruptFlag;
	saver["SB"] = SB;
	saver["SC"] = SC;
	saver["TIMA"] = TIMA;
	saver["TMA"] = TMA;
	saver["TAC"] = TAC;
	saver["lastTimer"] = lastTimer;
	saver["timaState"] = timaState;
	saver["FF72"] = FF72;
	saver["FF73"] = FF73;
	saver["FF74"] = FF74;
	saver["FF75"] = FF75;
	saver["HDMA1"] = HDMA1;
	saver["HDMA2"] = HDMA2;
	saver["HDMA3"] = HDMA3;
	saver["HDMA4"] = HDMA4;
	saver["speed"] = speed;
	saver["RP"] = RP;
	saver["BGPI"] = BGPI;
	saver["OBPI"] = OBPI;
	saver["vramBank"] = vramBank;
	saver["JoyPadSelect"] = JoyPadSelect;
	saver["inBios"] = inBios;
	saver["dmaCycles"] = dmaCycles;
	saver["dmaSrc"] = dmaSrc;
	saver["dmaDest"] = dmaDest;
	saver["dmaReg"] = dmaReg;
}

void Gameboy::LoadState(const nlohmann::json& saver) {
	cpu.LoadState(saver["cpu"]);
	ppu.LoadState(saver["ppu"]);
	apu.LoadState(saver["apu"]);
	mbc->LoadState(saver["mbc"]);

	DIV = saver["DIV"];
	ram = saver["ram"];
	hram = saver["hram"];
	ramBank = saver["ramBank"];
	InterruptEnable = saver["InterruptEnable"];
	InterruptFlag = saver["InterruptFlag"];
	SB = saver["SB"];
	SC = saver["SC"];
	TIMA = saver["TIMA"];
	TMA = saver["TMA"];
	TAC = saver["TAC"];
	lastTimer = saver["lastTimer"];
	timaState = saver["timaState"];
	FF72 = saver["FF72"];
	FF73 = saver["FF73"];
	FF74 = saver["FF74"];
	FF75 = saver["FF75"];
	HDMA1 = saver["HDMA1"];
	HDMA2 = saver["HDMA2"];
	HDMA3 = saver["HDMA3"];
	HDMA4 = saver["HDMA4"];
	speed = saver["speed"];
	RP = saver["RP"];
	BGPI = saver["BGPI"];
	OBPI = saver["OBPI"];
	vramBank = saver["vramBank"];
	JoyPadSelect = saver["JoyPadSelect"];
	inBios = saver["inBios"];
	dmaCycles = saver["dmaCycles"];
	dmaSrc = saver["dmaSrc"];
	dmaDest = saver["dmaDest"];
	dmaReg = saver["dmaReg"];
}

void Gameboy::clockTimer() {
	if(cpu.state == CpuState::Stop) return;

	const uint16_t rates[] = { 1 << 9, 1 << 3, 1 << 5, 1 << 7 };
	DIV += 4;

	if(timaState == GB_TIMA_RELOADED) {
		timaState = GB_TIMA_RUNNING;
	} else if(timaState == GB_TIMA_RELOADING) {
		timaState = GB_TIMA_RELOADED;
		Interrupt(Interrupt::Timer);
		TIMA = TMA;
	}

	auto timer = (TAC & 4) && DIV & rates[TAC & 3];
	if(!timer && lastTimer) {
		// falling edge
		TIMA++;
		if(TIMA == 0) {
			timaState = GB_TIMA_RELOADING;
		}
	}
	lastTimer = timer;
}

void Gameboy::clockDma(int cycles) {
	dmaCycles += cycles;

	while(dmaCycles >= 4 && dmaDest < 0xA0) {
		dmaCycles -= 4;
		ppu.OAM[dmaDest] = CpuRead(dmaSrc < 0xE000 ? dmaSrc : dmaSrc & ~0x2000);
		dmaSrc++;
		dmaDest++;
	}
}

}
