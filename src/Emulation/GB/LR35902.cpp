#include "LR35902.h"

#include <cassert>

#include "Gameboy.h"

// #define printDebug 1

namespace Gameboy {

#ifdef printDebug
// clang-format off
const char* normalNames[256] = {
	"NOP",        "LD BC,d16", "LD (BC),A",  "INC BC",    "INC B",       "DEC B",     "LD B,d8",    "RLCA",      "LD (a16),SP", "ADD HL,BC", "LD A,(BC)",  "DEC BC",    "INC C",      "DEC C",    "LD C,d8",    "RRCA",
	"STOP 0",     "LD DE,d16", "LD (DE),A",  "INC DE",    "INC D",       "DEC D",     "LD D,d8",    "RLA",       "JR r8",       "ADD HL,DE", "LD A,(DE)",  "DEC DE",    "INC E",      "DEC E",    "LD E,d8",    "RRA",
	"JR NZ,r8",   "LD HL,d16", "LD (HL+),A", "INC HL",    "INC H",       "DEC H",     "LD H,d8",    "DAA",       "JR Z,r8",     "ADD HL,HL", "LD A,(HL+)", "DEC HL",    "INC L",      "DEC L",    "LD L,d8",    "CPL",
	"JR NC,r8",   "LD SP,d16", "LD (HL-),A", "INC SP",    "INC (HL)",    "DEC (HL)",  "LD (HL),d8", "SCF",       "JR C,r8",     "ADD HL,SP", "LD A,(HL-)", "DEC SP",    "INC A",      "DEC A",    "LD A,d8",    "CCF",
	"LD B,B",     "LD B,C",    "LD B,D",     "LD B,E",    "LD B,H",      "LD B,L",    "LD B,(HL)",  "LD B,A",    "LD C,B",      "LD C,C",    "LD C,D",     "LD C,E",    "LD C,H",     "LD C,L",   "LD C,(HL)",  "LD C,A",
	"LD D,B",     "LD D,C",    "LD D,D",     "LD D,E",    "LD D,H",      "LD D,L",    "LD D,(HL)",  "LD D,A",    "LD E,B",      "LD E,C",    "LD E,D",     "LD E,E",    "LD E,H",     "LD E,L",   "LD E,(HL)",  "LD E,A",
	"LD H,B",     "LD H,C",    "LD H,D",     "LD H,E",    "LD H,H",      "LD H,L",    "LD H,(HL)",  "LD H,A",    "LD L,B",      "LD L,C",    "LD L,D",     "LD L,E",    "LD L,H",     "LD L,L",   "LD L,(HL)",  "LD L,A",
	"LD (HL),B",  "LD (HL),C", "LD (HL),D",  "LD (HL),E", "LD (HL),H",   "LD (HL),L", "HALT",       "LD (HL),A", "LD A,B",      "LD A,C",    "LD A,D",     "LD A,E",    "LD A,H",     "LD A,L",   "LD A,(HL)",  "LD A,A",
	"ADD A,B",    "ADD A,C",   "ADD A,D",    "ADD A,E",   "ADD A,H",     "ADD A,L",   "ADD A,(HL)", "ADD A,A",   "ADC A,B",     "ADC A,C",   "ADC A,D",    "ADC A,E",   "ADC A,H",    "ADC A,L",  "ADC A,(HL)", "ADC A,A",
	"SUB B",      "SUB C",     "SUB D",      "SUB E",     "SUB H",       "SUB L",     "SUB (HL)",   "SUB A",     "SBC A,B",     "SBC A,C",   "SBC A,D",    "SBC A,E",   "SBC A,H",    "SBC A,L",  "SBC A,(HL)", "SBC A,A",
	"AND B",      "AND C",     "AND D",      "AND E",     "AND H",       "AND L",     "AND (HL)",   "AND A",     "XOR B",       "XOR C",     "XOR D",      "XOR E",     "XOR H",      "XOR L",    "XOR (HL)",   "XOR A",
	"OR B",       "OR C",      "OR D",       "OR E",      "OR H",        "OR L",      "OR (HL)",    "OR A",      "CP B",        "CP C",      "CP D",       "CP E",      "CP H",       "CP L",     "CP (HL)",    "CP A",
	"RET NZ",     "POP BC",    "JP NZ,a16",  "JP a16",    "CALL NZ,a16", "PUSH BC",   "ADD A,d8",   "RST 00H",   "RET Z",       "RET",       "JP Z,a16",   "PREFIX CB", "CALL Z,a16", "CALL a16", "ADC A,d8",   "RST 08H",
	"RET NC",     "POP DE",    "JP NC,a16",  "",          "CALL NC,a16", "PUSH DE",   "SUB d8",     "RST 10H",   "RET C",       "RETI",      "JP C,a16",   " ",         "CALL C,a16", " ",        "SBC A,d8",   "RST 18H",
	"LDH (a8),A", "POP HL",    "LD (C),A",   "",          "",            "PUSH HL",   "AND d8",     "RST 20H",   "ADD SP,r8",   "JP (HL)",   "LD (a16),A", " ",         " ",          " ",        "XOR d8",     "RST 28H",
	"LDH A,(a8)", "POP AF",    "LD A,(C)",   "DI",        "",            "PUSH AF",   "OR d8",      "RST 30H",   "LD HL,SP+r8", "LD SP,HL",  "LD A,(a16)", "EI",        " ",          " ",        "CP d8",      "RST 38H"
};

const char* PrefixNames[] = {
	"RLC B",   "RLC C",   "RLC D",   "RLC E",   "RLC H",   "RLC L",   "RLC (HL)",   "RLC A",   "RRC B",   "RRC C",   "RRC D",   "RRC E",   "RRC H",   "RRC L",   "RRC (HL)",   "RRC A",
	"RL B",    "RL C",    "RL D",    "RL E",    "RL H",    "RL L",    "RL (HL)",    "RL A",    "RR B",    "RR C",    "RR D",    "RR E",    "RR H",    "RR L",    "RR (HL)",    "RR A",
	"SLA B",   "SLA C",   "SLA D",   "SLA E",   "SLA H",   "SLA L",   "SLA (HL)",   "SLA A",   "SRA B",   "SRA C",   "SRA D",   "SRA E",   "SRA H",   "SRA L",   "SRA (HL)",   "SRA A",
	"SWAP B",  "SWAP C",  "SWAP D",  "SWAP E",  "SWAP H",  "SWAP L",  "SWAP (HL)",  "SWAP A",  "SRL B",   "SRL C",   "SRL D",   "SRL E",   "SRL H",   "SRL L",   "SRL (HL)",   "SRL A",
	"BIT 0,B", "BIT 0,C", "BIT 0,D", "BIT 0,E", "BIT 0,H", "BIT 0,L", "BIT 0,(HL)", "BIT 0,A", "BIT 1,B", "BIT 1,C", "BIT 1,D", "BIT 1,E", "BIT 1,H", "BIT 1,L", "BIT 1,(HL)", "BIT 1,A",
	"BIT 2,B", "BIT 2,C", "BIT 2,D", "BIT 2,E", "BIT 2,H", "BIT 2,L", "BIT 2,(HL)", "BIT 2,A", "BIT 3,B", "BIT 3,C", "BIT 3,D", "BIT 3,E", "BIT 3,H", "BIT 3,L", "BIT 3,(HL)", "BIT 3,A",
	"BIT 4,B", "BIT 4,C", "BIT 4,D", "BIT 4,E", "BIT 4,H", "BIT 4,L", "BIT 4,(HL)", "BIT 4,A", "BIT 5,B", "BIT 5,C", "BIT 5,D", "BIT 5,E", "BIT 5,H", "BIT 5,L", "BIT 5,(HL)", "BIT 5,A",
	"BIT 6,B", "BIT 6,C", "BIT 6,D", "BIT 6,E", "BIT 6,H", "BIT 6,L", "BIT 6,(HL)", "BIT 6,A", "BIT 7,B", "BIT 7,C", "BIT 7,D", "BIT 7,E", "BIT 7,H", "BIT 7,L", "BIT 7,(HL)", "BIT 7,A",
	"RES 0,B", "RES 0,C", "RES 0,D", "RES 0,E", "RES 0,H", "RES 0,L", "RES 0,(HL)", "RES 0,A", "RES 1,B", "RES 1,C", "RES 1,D", "RES 1,E", "RES 1,H", "RES 1,L", "RES 1,(HL)", "RES 1,A",
	"RES 2,B", "RES 2,C", "RES 2,D", "RES 2,E", "RES 2,H", "RES 2,L", "RES 2,(HL)", "RES 2,A", "RES 3,B", "RES 3,C", "RES 3,D", "RES 3,E", "RES 3,H", "RES 3,L", "RES 3,(HL)", "RES 3,A",
	"RES 4,B", "RES 4,C", "RES 4,D", "RES 4,E", "RES 4,H", "RES 4,L", "RES 4,(HL)", "RES 4,A", "RES 5,B", "RES 5,C", "RES 5,D", "RES 5,E", "RES 5,H", "RES 5,L", "RES 5,(HL)", "RES 5,A",
	"RES 6,B", "RES 6,C", "RES 6,D", "RES 6,E", "RES 6,H", "RES 6,L", "RES 6,(HL)", "RES 6,A", "RES 7,B", "RES 7,C", "RES 7,D", "RES 7,E", "RES 7,H", "RES 7,L", "RES 7,(HL)", "RES 7,A",
	"SET 0,B", "SET 0,C", "SET 0,D", "SET 0,E", "SET 0,H", "SET 0,L", "SET 0,(HL)", "SET 0,A", "SET 1,B", "SET 1,C", "SET 1,D", "SET 1,E", "SET 1,H", "SET 1,L", "SET 1,(HL)", "SET 1,A",
	"SET 2,B", "SET 2,C", "SET 2,D", "SET 2,E", "SET 2,H", "SET 2,L", "SET 2,(HL)", "SET 2,A", "SET 3,B", "SET 3,C", "SET 3,D", "SET 3,E", "SET 3,H", "SET 3,L", "SET 3,(HL)", "SET 3,A",
	"SET 4,B", "SET 4,C", "SET 4,D", "SET 4,E", "SET 4,H", "SET 4,L", "SET 4,(HL)", "SET 4,A", "SET 5,B", "SET 5,C", "SET 5,D", "SET 5,E", "SET 5,H", "SET 5,L", "SET 5,(HL)", "SET 5,A",
	"SET 6,B", "SET 6,C", "SET 6,D", "SET 6,E", "SET 6,H", "SET 6,L", "SET 6,(HL)", "SET 6,A", "SET 7,B", "SET 7,C", "SET 7,D", "SET 7,E", "SET 7,H", "SET 7,L", "SET 7,(HL)", "SET 7,A"
};
// clang-format on
#endif

static int FirstBit(uint8_t val) {
	assert(val != 0);

	if(val & 1) return 0;
	if(val & 2) return 1;
	if(val & 4) return 2;
	if(val & 8) return 3;
	return 4;
}

void LR35902::Reset(Mode mode) {
#if false
	if(useBoot) {
		std::memset(reg, 0, 8);

		SP = 0;
		PC = 0;
	}
#endif

	SP = 0xFFFE;
	PC = 0x100;
	IME = true;

	auto cgbFlag = bus.CpuRead(0x0143);
	auto checkSum = bus.CpuRead(0x014D);
	auto oldLicense = bus.CpuRead(0x014B);
	
	uint8_t titleSum = 0;
	if((oldLicense == 1 || oldLicense == 33) && bus.CpuRead(0x0144) == '0' && bus.CpuRead(0x0145) == '1') {
		for(int i = 0x0134; i <= 0x0143; i++) {
			titleSum += bus.CpuRead(i);
		}
	}

	switch(mode) {
		case Mode::DMG0:
			AF = 0x0100;
			BC = 0xFF13;
			DE = 0x00C1;
			HL = 0x8403;
			break;
		case Mode::DMG:
		case Mode::MGB: 
			AF = 0x0180;
			F.H = F.C = checkSum != 0;
			BC = 0x0013;
			DE = 0x00D8;
			HL = 0x014D;
			break;
		case Mode::SGB:
			AF = 0x0100;
			BC = 0x0014;
			DE = 0x0000;
			HL = 0xC060;
			break;
		case Mode::SGB2:
			AF = 0xFF00;
			BC = 0x0014;
			DE = 0x0000;
			HL = 0xC060;
			break;
		case Mode::CGB:
			if(cgbFlag == 0x80 || cgbFlag == 0xC0) {
				AF = 0x1180;
				BC = 0x0000;
				DE = 0xFF56;
				HL = 0x000D;
			} else { // DMG mode
				AF = 0x1180;
				B = titleSum;
				C = 0x00;
				DE = 0x0008;
				HL = B == 43 || B == 58 ? 0x991A : 0x007C;
			}
			break;
		case Mode::AGB:
			AF = 0x1100;
			if(cgbFlag == 0x80 || cgbFlag == 0xC0) {
				BC = 0x0100;
				DE = 0xFF56;
				HL = 0x000D;
			} else { // DMG mode
				F.H = (titleSum & 0xF) == 0xF;
				B = titleSum + 1;
				F.Z = B == 0;
				C = 0x00;
				DE = 0x0008;
				HL = B == 44 || B == 59 ? 0x991A : 0x007C;
				break;
			}
			break;
	}

	IMEtoggle = false;
	state = CpuState::Normal;
}

bool LR35902::checkCond(uint8_t opcode) const {
	switch((opcode >> 3) & 3) {
		case 0: return !F.Z;
		case 1: return F.Z;
		case 2: return !F.C;
		case 3: return F.C;
	}

	// unreachable
	assert(false);
	return false;
}

uint8_t LR35902::RightReg(uint8_t opcode) {
	switch(opcode & 7) {
		case 0: return B;
		case 1: return C;
		case 2: return D;
		case 3: return E;
		case 4: return H;
		case 5: return L;
		case 6: return read(HL);
		case 7: return A;
	}

	// unreachable
	assert(false);
	return 0;
}

void LR35902::WriteRightReg(uint8_t opcode, uint8_t val) {
	switch(opcode & 7) {
		case 0: B = val; break;
		case 1: C = val; break;
		case 2: D = val; break;
		case 3: E = val; break;
		case 4: H = val; break;
		case 5: L = val; break;
		case 6: write(HL, val); break;
		case 7: A = val; break;
	}
}

uint16_t LR35902::GetBigReg(uint8_t id) const {
	switch(id & 3) {
		case 0: return BC;
		case 1: return DE;
		case 2: return HL;
		case 3: return SP;
	}

	// unreachable
	assert(false);
	return 0;
}

void LR35902::SetBigReg(uint8_t id, uint16_t val) {
	switch(id & 3) {
		case 0: BC = val; break;
		case 1: DE = val; break;
		case 2: HL = val; break;
		case 3: SP = val; break;
	}
}

uint16_t LR35902::Imm16() {
	return read(PC++) | (read(PC++) << 8);
}

void LR35902::Step() {
	auto interrupt = bus.InterruptEnable & bus.InterruptFlag;

	if(state == CpuState::Stop) {
		bus.Advance();
		bus.pendingCycles += 4;

		if((read(0xFF00) & 0xF) != 0xF) state = CpuState::Normal;
		else return;
	} else if(state == CpuState::Halt) {
		bus.Advance();
		bus.pendingCycles += 4;

		if(interrupt) state = CpuState::Normal;
		else return;
	}

	if(interrupt && IME) {
		auto id = FirstBit(interrupt);
#ifdef printDebug
		printf("Interrupt %i\n", id);
#endif

		IME = false;

		bus.InterruptFlag &= ~(1 << id);

		cycleStall();
		cycleStall();
		cycleStall();

		write(--SP, PC >> 8);
		write(--SP, PC);

		PC = 0x40 + (id << 3);
		return;
	}

	if(IMEtoggle) {
		IME = !IME;
		IMEtoggle = false;
	}

	auto opcode = read(PC++);
	if(state == CpuState::HaltBug) {
		PC--;
	}
	state = CpuState::Normal;

#ifdef printDebug
	if(opcode != 0xCB) {
		auto immLow = bus.CpuRead(PC);

		printf("%04X ", PC - 1);
		auto str = normalNames[opcode];
		auto len = strlen(str);
		auto printed = 0;
		for(size_t i = 0; i < len; i++) {
			auto c = str[i];
			if(c == 'r') {
				printed += printf("%i", (int8_t)immLow);
				i++;
			} else if(c == 'a' || c == 'd') {
				i++;
				auto next = str[i];
				if(next == '8') {
					printed += printf("%02X", immLow);
				} else {
					i++;
					auto immHigh = bus.CpuRead(PC + 1);
					printed += printf("%04X", immLow | immHigh << 8);
				}
			} else {
				printed++;
				putchar(c);
			}
		}
		while(printed < 12) {
			putchar(' ');
			printed++;
		}

		// putchar('\n');
		printf(" BC=%02X%02X DE=%02X%02X HL=%02X%02X AF=%02X%02X SP=%04X", B, C, D, E, H, L, A, F.reg, SP);
		printf(" LY=%02X DIV=%02X TIMA=%02X\n", bus.CpuRead(0xFF44), bus.DIV >> 8, bus.TIMA);
	}
#endif
	
	switch(opcode) {
#pragma region control/misc
		case 0x00: break; // NOP
		case 0x10: // stop
			if(read(0xFF00) & 0xF != 0xF) {
				if(!interrupt) {
					PC++;
					state = CpuState::Halt;
				}
			} else {
				if(bus.speed & 1) {
					if(interrupt) {
						if(IME) {
							// bugged
						} else {
							bus.DIV = 0;
							bus.speed = (bus.speed ^ 0x80) & 0xFE;
						}
					} else {
						PC++;
						// unless an interrupt occurs before then HALT mode will exit automatically after about 0x20000 T-cycles
						state = CpuState::Stop;
						for(int i = 0; i < 33941; ++i) {
							bus.Advance();
							bus.pendingCycles += 4;
							if(bus.InterruptEnable & bus.InterruptFlag) {
								break;
							}
						}
						state = CpuState::Normal;
						bus.speed = (bus.speed ^ 0x80) & 0xFE;
						bus.DIV = 0;
					}
				} else {
					if(!interrupt) PC++;
					state = CpuState::Stop;
					bus.DIV = 0;
				}
			}
			break;
		case 0x76: // halt
			if(interrupt) {
				if(IME) {
					state = CpuState::Normal;
					PC--;
				} else {
					state = CpuState::HaltBug;
				}
			} else {
				state = CpuState::Halt;
			}
			// TODO: justHalted = true;
			break;
		case 0xCB: Prefix(); break; // PREFIX CB
		case 0xF3: IME = false; break; // DI
		case 0xFB: if(!IME) IMEtoggle = true; break; // EI
#pragma endregion

#pragma region control/branch
		case 0x18: // JR
			PC += static_cast<int8_t>(read(PC)) + 1;
			cycleStall();
			break;
		case 0x20:	 // JR NZ,r8
		case 0x28:	 // JR Z ,r8
		case 0x30:	 // JR NC,r8
		case 0x38: { // JR C ,r8
			int8_t val = read(PC++);
			if(checkCond(opcode)) {
				PC += val;
				cycleStall();
			}
			break;
		}
		case 0xC2: case 0xCA: case 0xD2: case 0xDA: { // JP rr,a16
			uint16_t temp = Imm16();

			if(checkCond(opcode)) {
				PC = temp;
				cycleStall();
			}
			break;
		}
		case 0xC3: // JP a16
			PC = Imm16();
			cycleStall();
			break;
		case 0xE9: // JP HL
			PC = HL;
			break;
		case 0xC7: case 0xCF: case 0xD7: case 0xDF: case 0xE7: case 0xEF: case 0xF7: case 0xFF: // RST
			write(--SP, PC >> 8);
			write(--SP, PC);

			PC = (opcode & 0b00111000);
			cycleStall();
			break;
		case 0xC4: case 0xCC: case 0xD4: case 0xDC: { // CALL cc,a16
			uint16_t temp = Imm16();
			if(checkCond(opcode)) {
				cycleOamBug(SP);
				write(--SP, PC >> 8);
				write(--SP, PC);
				PC = temp;
			}
			break;
		}
		case 0xCD: { // CALL a16
			uint16_t temp = Imm16();
			cycleOamBug(SP);
			write(--SP, PC >> 8);
			write(--SP, PC);
			PC = temp;
			break;
		}
		case 0xC0: // RET NZ
		case 0xC8: // RET Z
		case 0xD0: // RET NC
		case 0xD8: // RET C
			if(checkCond(opcode)) {
				cycleStall();
				PC = read(SP++);
				PC |= read(SP++) << 8;
			}
			cycleStall();
			break;
		case 0xC9: // RET
			PC = read(SP++);
			PC |= read(SP++) << 8;
			cycleStall();
			break;
		case 0xD9: // RETI
			PC = read(SP++);
			PC |= read(SP++) << 8;
			cycleStall();
			IME = true;
			break;
#pragma endregion

#pragma region 8-bit Load/Store/Move
		case 0x06: case 0x0E:
		case 0x16: case 0x1E:
		case 0x26: case 0x2E:
		case 0x36: case 0x3E: // LD r,d8
			WriteRightReg(opcode >> 3, read(PC++));
			break;

		case 0x40: case 0x41: case 0x42: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47: case 0x48: case 0x49: case 0x4A: case 0x4B: case 0x4C: case 0x4D: case 0x4E: case 0x4F:
		case 0x50: case 0x51: case 0x52: case 0x53: case 0x54: case 0x55: case 0x56: case 0x57: case 0x58: case 0x59: case 0x5A: case 0x5B: case 0x5C: case 0x5D: case 0x5E: case 0x5F:
		case 0x60: case 0x61: case 0x62: case 0x63: case 0x64: case 0x65: case 0x66: case 0x67: case 0x68: case 0x69: case 0x6A: case 0x6B: case 0x6C: case 0x6D: case 0x6E: case 0x6F:
		case 0x70: case 0x71: case 0x72: case 0x73: case 0x74: case 0x75:            case 0x77: case 0x78: case 0x79: case 0x7A: case 0x7B: case 0x7C: case 0x7D: case 0x7E: case 0x7F:
			// LD r, r
			WriteRightReg(opcode >> 3, RightReg(opcode));
			break;

		case 0x02: write(BC, A); break; // LD (BC),A
		case 0x12: write(DE, A); break; // LD (DE),A
		case 0x22: write(HL++, A); break; // LD (HL+),A
		case 0x32: write(HL--, A); break; // LD (HL-),A
		case 0xE0: write(0xFF00 | read(PC++), A); break; // LDH (a8),A
		case 0xE2: write(0xFF00 | C, A); break; // LD (C),A
		case 0xEA: write(Imm16(), A); break; // LD (a16),A

		case 0x0A: A = read(BC); break; // LD A,(BC)
		case 0x1A: A = read(DE); break; // LD A,(DE)
		case 0x2A: A = read(HL++); break; // LD A,(HL+)
		case 0x3A: A = read(HL--); break; // LD A,(HL-)
		case 0xF0: A = read(0xFF00 | read(PC++)); break; // LDH A,(a8)
		case 0xF2: A = read(0xFF00 | C); break; // LD A,(C)
		case 0xFA: A = read(Imm16()); break; // LD A,(a16)
#pragma endregion

#pragma region 16-bit Load/Store/Move
		case 0x01: // __debugbreak();
		case 0x11:
		case 0x21:
		case 0x31: // LD rr,d16
			SetBigReg(opcode >> 4, Imm16());
			break;
		case 0x08: { // LD (a16),SP
			uint16_t temp = Imm16();
			write(temp, SP);
			write(temp + 1, SP >> 8);
			break;
		}
		case 0xC5: case 0xD5: case 0xE5: case 0xF5: // PUSH rr
			cycleOamBug(SP);
			write(--SP, reg[((opcode >> 3) & 6) | 0]);
			write(--SP, reg[((opcode >> 3) & 6) | 1]);
			break;
		case 0xC1: case 0xD1: case 0xE1: // POP rr
			reg[((opcode >> 3) & 6) | 1] = read(SP++);
			reg[((opcode >> 3) & 6) | 0] = read(SP++);
			break;
		case 0xF1: // POP AF
			// needs extra case cause F only has 4 bits
			F.reg = read(SP++) & 0xF0;
			A = read(SP++);
			break;
		case 0xF9: // LD SP,HL
			SP = HL;
			cycleOamBug(HL);
			break;
#pragma endregion

#pragma region 8-bit Arithmetic Logic Unit
		case 0x04: case 0x0C:
		case 0x14: case 0x1C:
		case 0x24: case 0x2C:
		case 0x34: case 0x3C: { // INC r
			auto r = RightReg(opcode >> 3);
			F.H = (r & 0xF) == 0xF;
			r++;
			WriteRightReg(opcode >> 3, r);
			F.Z = r == 0;
			F.N = false;
			break;
		}
		case 0x05: case 0x0D:
		case 0x15: case 0x1D:
		case 0x25: case 0x2D:
		case 0x35: case 0x3D: { // DEC r
			auto r = RightReg(opcode >> 3);
			F.H = (r & 0xF) == 0;
			r--;
			WriteRightReg(opcode >> 3, r);
			F.Z = r == 0;
			F.N = true;
			break;
		}
		case 0x27: // DAA
			// http://forums.nesdev.com/viewtopic.php?f=20&t=15944
			if(!F.N) { // after an addition, adjust if (half-)carry occurred or if result is out of bounds
				if(F.C || A > 0x99) {
					A += 0x60;
					F.C = true;
				}
				if(F.H || (A & 0x0F) > 0x09) A += 0x6;
			} else { // after a subtraction, only adjust if (half-)carry occurred
				if(F.C) A -= 0x60;
				if(F.H) A -= 0x6;
			}
			F.Z = A == 0;
			F.H = false;
			break;
		case 0x2F: // CPL
			A ^= 0xFF;
			F.N = true;
			F.H = true;
			break;
		case 0x37: // SCF
			F.N = false;
			F.H = false;
			F.C = true;
			break;
		case 0x3F: // CCF
			F.N = false;
			F.H = false;
			F.C = !F.C;
			break;
		case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: case 0x86: case 0x87: { // ADD r
			auto temp = RightReg(opcode);

			F.H = (A & 0xF) + (temp & 0xF) > 0xF;
			F.C = A + temp > 0xFF;
			A += temp;
			F.Z = A == 0;
			F.N = false;
			break;
		}
		case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8E: case 0x8F: { // ADC r
			auto temp = RightReg(opcode);
			auto c = F.C;

			F.H = (A & 0xF) + (temp & 0xF) + c > 0xF;
			F.C = A + temp + c > 0xFF;
			A += temp + c;

			F.Z = A == 0;
			F.N = false;
			break;
		}
		case 0x90: case 0x91: case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: case 0x97: { // SUB r
			auto temp = RightReg(opcode);

			F.H = (A & 0xF) < (temp & 0xF);
			F.C = A < temp;
			A -= temp;
			F.Z = A == 0;
			F.N = true;
			break;
		}
		case 0x98: case 0x99: case 0x9A: case 0x9B: case 0x9C: case 0x9D: case 0x9E: case 0x9F: { // SBC r
			auto temp = RightReg(opcode);
			auto c = F.C;

			F.H = (A & 0xF) < (temp & 0xF) + c;
			F.C = unsigned(A - temp - c) > 0xFF;
			A -= temp + c;

			F.Z = A == 0;
			F.N = true;
			break;
		}
		case 0xA0: case 0xA1: case 0xA2: case 0xA3: case 0xA4: case 0xA5: case 0xA6: case 0xA7: // AND r
			A &= RightReg(opcode);
			F.Z = A == 0;
			F.N = false;
			F.H = true;
			F.C = false;
			break;
		case 0xA8: case 0xA9: case 0xAA: case 0xAB: case 0xAC: case 0xAD: case 0xAE: case 0xAF: // XOR r
			A ^= RightReg(opcode);
			F.Z = A == 0;
			F.N = false;
			F.H = false;
			F.C = false;
			break;
		case 0xB0: case 0xB1: case 0xB2: case 0xB3: case 0xB4: case 0xB5: case 0xB6: case 0xB7: // OR r
			A |= RightReg(opcode);
			F.Z = A == 0;
			F.N = false;
			F.H = false;
			F.C = false;
			break;
		case 0xB8: case 0xB9: case 0xBA: case 0xBB: case 0xBC: case 0xBD: case 0xBE: case 0xBF: { // CP r
			auto temp = RightReg(opcode);
			F.Z = A == temp;
			F.N = true;
			F.H = (A & 0xF) < (temp & 0xF);
			F.C = A < temp;
			break;
		}
		case 0xC6: { // ADD A,u8
			auto temp = read(PC++);

			F.H = (A & 0xF) + (temp & 0xF) > 0xF;
			F.C = A + temp > 0xFF;
			A += temp;
			F.Z = A == 0;
			F.N = false;
			break;
		}
		case 0xCE: { // ADC A,d8
			auto temp = read(PC++);
			auto c = F.C;

			F.H = (A & 0xF) + (temp & 0xF) + c > 0xF;
			F.C = A + temp + c > 0xFF;
			A += temp + c;

			F.Z = A == 0;
			F.N = false;
			break;
		}
		case 0xD6: { // SUB d8
			auto temp = read(PC++);

			F.H = (A & 0xF) < (temp & 0xF);
			F.C = A < temp;
			A -= temp;
			F.Z = A == 0;
			F.N = true;
			break;
		}
		case 0xDE: { // SBC A,d8
			auto temp = read(PC++);
			auto c = F.C;

			F.H = (A & 0xF) < (temp & 0xF) + c;
			F.C = unsigned(A - temp - c) > 0xFF;
			A -= temp + c;

			F.Z = A == 0;
			F.N = true;
			break;
		}
		case 0xE6: // AND d8
			A &= read(PC++);
			F.Z = A == 0;
			F.N = false;
			F.H = true;
			F.C = false;
			break;
		case 0xEE: // XOR d8
			A ^= read(PC++);
			F.Z = A == 0;
			F.N = false;
			F.H = false;
			F.C = false;
			break;
		case 0xF6: // OR d8
			A |= read(PC++);
			F.Z = A == 0;
			F.N = false;
			F.H = false;
			F.C = false;
			break;
		case 0xFE: { // CP d8
			uint8_t temp = read(PC++);
			F.Z = A == temp;
			F.N = true;
			F.H = (A & 0xF) < (temp & 0xF);
			F.C = A < temp;
			break;
		}
#pragma endregion

#pragma region 16-bit Arithmetic Logic Unit
		case 0x09: case 0x19:
		case 0x29: case 0x39: { // ADD HL,rr
			cycleStall();
			auto val = GetBigReg(opcode >> 4);
			F.H = (HL & 0xFFF) + (val & 0xFFF) > 0xFFF;
			F.C = HL + val > 0xFFFF;
			HL += val;
			F.N = false;
			break;
		}
		case 0x03: case 0x13:
		case 0x23: case 0x33: {// INC rr
			auto reg = GetBigReg(opcode >> 4) ;
			cycleOamBug(reg);
			SetBigReg(opcode >> 4, reg + 1);
			break;
		}
		case 0x0B: case 0x1B:
		case 0x2B: case 0x3B: {// DEC rr
			auto reg = GetBigReg(opcode >> 4) ;
			cycleOamBug(reg);
			SetBigReg(opcode >> 4, reg - 1);
			break;
		}
		case 0xE8: { // ADD SP,i8
			auto temp = (int8_t)read(PC++);
			cycleStall();
		    cycleStall();
		
			F.H = (temp & 0xF) + (SP & 0xF) > 0xF;
			F.C = (temp & 0xFF) + (SP & 0xFF) > 0xFF;
			F.Z = false;
			F.N = false;

			SP += temp;
			break;
		}
		case 0xF8: { // LD HL,SP+r8
			int val = static_cast<int8_t>(read(PC++));
			cycleStall();

			HL = SP + val;
			F.Z = false;
			F.N = false;

			F.H = (val & 0xF) + (SP & 0xF) > 0xF;
			F.C = (val & 0xFF) + (SP & 0xFF) > 0xFF;
			break;
		}
#pragma endregion

		case 0x07: // RLCA
			A = (A << 1) | (A >> 7);
			F.Z = false;
			F.N = false;
			F.H = false;
			F.C = A & 1;
			break;
		case 0x0F: // RRCA
			F.C = A & 1;
			A = (A >> 1) | (A << 7);
			F.Z = false;
			F.N = false;
			F.H = false;
			break;
		case 0x017: { // RLA
			auto temp = A;
			A = A << 1 | F.C;
			F.Z = false;
			F.N = false;
			F.H = false;
			F.C = temp >> 7;
			break;
		}
		case 0x1F: { // RRA
			auto temp = A;
			A = (F.C << 7) | (A >> 1);
			F.Z = false;
			F.N = false;
			F.H = false;
			F.C = temp & 1;
			break;
		}
		// default: throw std::runtime_error("Illegal opcode");
	}
}

void LR35902::Prefix() {
	auto opcode = read(PC++);
	auto fetched = RightReg(opcode);

	#ifdef printDebug
	printf("%04X %-12s BC=%02X%02X DE=%02X%02X HL=%02X%02X AF=%02X%02X SP=%04X", PC - 2, PrefixNames[opcode], B,C, D,E, H,L, A,F, SP);
	printf(" LY=%02X DIV=%02X TIMA=%02X\n", cycle_read(0xFF44), bus.DIV >> 8, bus.TIMA);
	// if(logging) logFile << hex16(PC - 1) << ' ' << PrefixNames[opcode] << '\n';
	#endif

	switch(opcode >> 6) {
		case 0:
			switch(opcode >> 3) {
				case 0: { // RLC r
					fetched = fetched << 1 | fetched >> 7;
					WriteRightReg(opcode, fetched);
					F.Z = fetched == 0;
					F.N = false;
					F.H = false;
					F.C = fetched & 1;
					break;
				}
				case 1: { // RRC r
					F.C = fetched & 1;
					fetched = (fetched << 7) | (fetched >> 1);
					WriteRightReg(opcode, fetched);
					F.Z = fetched == 0;
					F.N = false;
					F.H = false;
					break;
				}
				case 2: { // RL r
					auto temp = F.C;
					F.C = fetched >> 7;
					fetched = (fetched << 1) | temp;
					WriteRightReg(opcode, fetched);
					F.Z = fetched == 0;
					F.N = false;
					F.H = false;
					break;
				}
				case 3: { // RR r
					auto temp = F.C;
					F.C = fetched & 1;
					fetched = (temp << 7) | (fetched >> 1);
					WriteRightReg(opcode, fetched);
					F.Z = fetched == 0;
					F.N = false;
					F.H = false;
					break;
				}
				case 4: // SLA r
					F.C = fetched >> 7;
					fetched <<= 1;
					WriteRightReg(opcode, fetched);
					F.Z = fetched == 0;
					F.N = false;
					F.H = false;
					break;
				case 5: // SRA r
					F.C = fetched & 1;
					fetched = (fetched & 0x80) | fetched >> 1;
					WriteRightReg(opcode, fetched);
					F.Z = fetched == 0;
					F.N = false;
					F.H = false;
					break;
				case 6: // SWAP r
					fetched = (fetched << 4) | (fetched >> 4);
					WriteRightReg(opcode, fetched);
					F.Z = fetched == 0;
					F.N = false;
					F.H = false;
					F.C = false;
					break;
				case 7: // SRL r
					F.C = fetched & 1;
					fetched = fetched >> 1;
					WriteRightReg(opcode, fetched);
					F.Z = fetched == 0;
					F.N = false;
					F.H = false;
					break;
			}
			break;
		case 1: // BIT n,r
			F.Z = !(fetched >> ((opcode >> 3) & 7) & 1);
			F.N = false;
			F.H = true;
			break;
		case 2: // RES n,r
			WriteRightReg(opcode, fetched & ~(1 << ((opcode >> 3) & 7)));
			break;
		case 3: // SET n,r
			WriteRightReg(opcode, fetched | (1 << ((opcode >> 3) & 7)));
			break;
	}
}

uint8_t LR35902::read(uint16_t addr) {
	bus.Advance();
	auto val = bus.CpuRead(addr);
	bus.pendingCycles += 4;
	return val;
}
void LR35902::write(uint16_t addr, uint8_t value) {
	bus.Advance();
	bus.CpuWrite(addr, value);
	bus.pendingCycles += 4;
}
void LR35902::cycleStall() {
	bus.pendingCycles += 4;
}
void LR35902::cycleOamBug(uint16_t value) {
	cycleStall();
}

void LR35902::SaveState(saver& saver) {
	saver << reg;
	saver << PC;
	saver << SP;
	saver << IMEtoggle;
	saver << IME;
	saver << state;
}

void LR35902::LoadState(saver& saver) {
	saver >> reg;
	saver >> PC;
	saver >> SP;
	saver >> IMEtoggle;
	saver >> IME;
	saver >> state;
}

}
