#include "mos6502.h"
#include "Bus.h"

// address mode of jsr changed
const Instruction lookup[] = {
	{BRK, IMP}, {ORA, IZX}, {KIL, IMM}, {SLO, IZX}, {NOP, ZP0}, {ORA, ZP0}, {ASL, ZP0}, {SLO, ZP0}, {PHP, IMP}, {ORA, IMM}, {ASL, IMP}, {ANC, IMM}, {NOP, ABS}, {ORA, ABS}, {ASL, ABS}, {SLO, ABS},
	{BPL, REL}, {ORA, IZY}, {KIL, IMM}, {SLO, IZY}, {NOP, ZPX}, {ORA, ZPX}, {ASL, ZPX}, {SLO, ZPX}, {CLC, IMP}, {ORA, ABY}, {NOP, IMP}, {SLO, ABY}, {NOP, ABX}, {ORA, ABX}, {ASL, ABX}, {SLO, ABX},
	{JSR, IMP}, {AND, IZX}, {KIL, IMM}, {RLA, IZX}, {BIT, ZP0}, {AND, ZP0}, {ROL, ZP0}, {RLA, ZP0}, {PLP, IMP}, {AND, IMM}, {ROL, IMP}, {ANC, IMM}, {BIT, ABS}, {AND, ABS}, {ROL, ABS}, {RLA, ABS},
	{BMI, REL}, {AND, IZY}, {KIL, IMM}, {RLA, IZY}, {NOP, ZPX}, {AND, ZPX}, {ROL, ZPX}, {RLA, ZPX}, {SEC, IMP}, {AND, ABY}, {NOP, IMP}, {RLA, ABY}, {NOP, ABX}, {AND, ABX}, {ROL, ABX}, {RLA, ABX},
	{RTI, IMP}, {EOR, IZX}, {KIL, IMM}, {SRE, IZX}, {NOP, ZP0}, {EOR, ZP0}, {LSR, ZP0}, {SRE, ZP0}, {PHA, IMP}, {EOR, IMM}, {LSR, IMP}, {ALR, IMM}, {JMP, ABS}, {EOR, ABS}, {LSR, ABS}, {SRE, ABS},
	{BVC, REL}, {EOR, IZY}, {KIL, IMM}, {SRE, IZY}, {NOP, ZPX}, {EOR, ZPX}, {LSR, ZPX}, {SRE, ZPX}, {CLI, IMP}, {EOR, ABY}, {NOP, IMP}, {SRE, ABY}, {NOP, ABX}, {EOR, ABX}, {LSR, ABX}, {SRE, ABX},
	{RTS, IMP}, {ADC, IZX}, {KIL, IMM}, {RRA, IZX}, {NOP, ZP0}, {ADC, ZP0}, {ROR, ZP0}, {RRA, ZP0}, {PLA, IMP}, {ADC, IMM}, {ROR, IMP}, {ARR, IMM}, {JMP, IND}, {ADC, ABS}, {ROR, ABS}, {RRA, ABS},
	{BVS, REL}, {ADC, IZY}, {KIL, IMM}, {RRA, IZY}, {NOP, ZPX}, {ADC, ZPX}, {ROR, ZPX}, {RRA, ZPX}, {SEI, IMP}, {ADC, ABY}, {NOP, IMP}, {RRA, ABY}, {NOP, ABX}, {ADC, ABX}, {ROR, ABX}, {RRA, ABX},
	{NOP, IMM}, {STA, IZX}, {NOP, IMM}, {SAX, IZX}, {STY, ZP0}, {STA, ZP0}, {STX, ZP0}, {SAX, ZP0}, {DEY, IMP}, {NOP, IMM}, {TXA, IMP}, {XAA, IMM}, {STY, ABS}, {STA, ABS}, {STX, ABS}, {SAX, ABS},
	{BCC, REL}, {STA, IZY}, {KIL, IMM}, {AHX, IZY}, {STY, ZPX}, {STA, ZPX}, {STX, ZPY}, {SAX, ZPY}, {TYA, IMP}, {STA, ABY}, {TXS, IMP}, {TAS, ABY}, {SHY, ABX}, {STA, ABX}, {SHX, ABY}, {AHX, ABY},
	{LDY, IMM}, {LDA, IZX}, {LDX, IMM}, {LAX, IZX}, {LDY, ZP0}, {LDA, ZP0}, {LDX, ZP0}, {LAX, ZP0}, {TAY, IMP}, {LDA, IMM}, {TAX, IMP}, {LAX, IMM}, {LDY, ABS}, {LDA, ABS}, {LDX, ABS}, {LAX, ABS},
	{BCS, REL}, {LDA, IZY}, {KIL, IMM}, {LAX, IZY}, {LDY, ZPX}, {LDA, ZPX}, {LDX, ZPY}, {LAX, ZPY}, {CLV, IMP}, {LDA, ABY}, {TSX, IMP}, {LAS, ABY}, {LDY, ABX}, {LDA, ABX}, {LDX, ABY}, {LAX, ABY},
	{CPY, IMM}, {CMP, IZX}, {NOP, IMM}, {DCP, IZX}, {CPY, ZP0}, {CMP, ZP0}, {DEC, ZP0}, {DCP, ZP0}, {INY, IMP}, {CMP, IMM}, {DEX, IMP}, {AXS, IMM}, {CPY, ABS}, {CMP, ABS}, {DEC, ABS}, {DCP, ABS},
	{BNE, REL}, {CMP, IZY}, {KIL, IMM}, {DCP, IZY}, {NOP, ZPX}, {CMP, ZPX}, {DEC, ZPX}, {DCP, ZPX}, {CLD, IMP}, {CMP, ABY}, {NOP, IMP}, {DCP, ABY}, {NOP, ABX}, {CMP, ABX}, {DEC, ABX}, {DCP, ABX},
	{CPX, IMM}, {SBC, IZX}, {NOP, IMM}, {ISC, IZX}, {CPX, ZP0}, {SBC, ZP0}, {INC, ZP0}, {ISC, ZP0}, {INX, IMP}, {SBC, IMM}, {NOP, IMP}, {SBC, IMM}, {CPX, ABS}, {SBC, ABS}, {INC, ABS}, {ISC, ABS},
	{BEQ, REL}, {SBC, IZY}, {KIL, IMM}, {ISC, IZY}, {NOP, ZPX}, {SBC, ZPX}, {INC, ZPX}, {ISC, ZPX}, {SED, IMP}, {SBC, ABY}, {NOP, IMP}, {ISC, ABY}, {NOP, ABX}, {SBC, ABX}, {INC, ABX}, {ISC, ABX},
};

static State InstructionType(Instructions instruction) {
	switch(instruction) {
			// Read
		case Instructions::LDA:
		case Instructions::LDX:
		case Instructions::LDY:
		case Instructions::EOR:
		case Instructions::AND:
		case Instructions::ORA:
		case Instructions::ADC:
		case Instructions::SBC:
		case Instructions::CMP:
		case Instructions::CPX:
		case Instructions::CPY:
		case Instructions::BIT:
		case Instructions::LAX:
		case Instructions::NOP:

			// Read write
		case Instructions::ASL:
		case Instructions::LSR:
		case Instructions::ROL:
		case Instructions::ROR:
		case Instructions::INC:
		case Instructions::DEC:
		case Instructions::SLO:
		case Instructions::SRE:
		case Instructions::RLA:
		case Instructions::RRA:
		case Instructions::ISC:
		case Instructions::DCP:
		case Instructions::LAS:
			return State::ReadInstr;
			// Write
		case Instructions::STA:
		case Instructions::STX:
		case Instructions::STY:
		case Instructions::SAX:
		case Instructions::AHX:
		case Instructions::SHX:
		case Instructions::SHY:
		case Instructions::TAS:
			return State::WriteInstr;
		default: throw std::logic_error("not reachable");
	}
}

mos6502::mos6502(Bus* bus): bus(bus) {
	#ifdef printDebug
	file.open("D:\\Daten\\Desktop\\test.log");
	#endif
}

mos6502::~mos6502() {
	#ifdef printDebug
	file.close();
	#endif
}

void mos6502::HardReset() {
	IRQ = false;
	NMI = false;

	A = 0;
	X = 0;
	Y = 0;
	SP = 0xFD;
	PC = bus->CpuRead(0xFFFC) | (bus->CpuRead(0xFFFD) << 8);
	Status.reg = 0b00100100;

	state = State::FetchOpcode;
}

void mos6502::Reset() {
	PC = bus->CpuRead(0xFFFC) | (bus->CpuRead(0xFFFD) << 8);
	SP -= 3;

	Status.I = Status.U = true;
	state = State::FetchOpcode;
	NMI = false;
}

void mos6502::Nmi() {
	NMI = true;
}

void mos6502::SaveState(saver& saver) const {
	saver << IRQ;
	saver << NMI;

	saver << A;
	saver << X;
	saver << Y;
	saver << SP;
	saver << PC;

	saver << Status.reg;
	saver.Write(&state, 1);

	saver << ptr;
	saver << addr_abs;

	saver.Write(&instruction.instruction, 1);
	saver.Write(&instruction.addrMode, 1);
}

void mos6502::LoadState(saver& saver) {
	saver >> IRQ;
	saver >> NMI;

	saver >> A;
	saver >> X;
	saver >> Y;
	saver >> SP;
	saver >> PC;

	saver >> Status.reg;
	saver.Read(&state, 1);

	saver >> ptr;
	saver >> addr_abs;

	saver.Read(&instruction.instruction, 1);
	saver.Read(&instruction.addrMode, 1);
}

void mos6502::Clock() {
	uint8_t fetched = 0;
	#ifdef printDebug
	static char instrStr[256] = "\0";
	static int strPos = 0;
	#endif

	switch(state) {
		case State::FetchOpcode:
			#ifdef printDebug
			if(strPos != 0) {
				for(int i = strPos; i < 27; ++i) {
					instrStr[i] = ' ';
				}

				// printf(instrStr);
				file << instrStr;
			}
			#endif

			fetched = bus->CpuRead(PC);
			if(NMI) {
				instruction.instruction = Instructions::NMI;
				instruction.addrMode = IMP;
			} else if(IRQ && !Status.I) {
				instruction.instruction = Instructions::IRQ;
				instruction.addrMode = IMP;
			} else {
				instruction = lookup[fetched];
				PC++;
			}

			#ifdef printDebug
			if(NMI || IRQ && !Status.I) {
				strPos = sprintf_s(instrStr, "%04X:  %02X  %s ", 0, fetched, InstructionNames[(int)instruction.instruction]);
			} else {
				strPos = sprintf_s(instrStr, "%04X:  %02X  %s ", PC - 1, fetched, InstructionNames[(int)instruction.instruction]);
			}
			sprintf_s((instrStr + 27), sizeof(instrStr) - 27, "A:%02X X:%02X Y:%02X P:%02X SP:%02X Cy:%i\n", A, X, Y, Status.reg, SP, bus->systemClockCounter + 1);
			#endif
			state = State::FetchOperator;
			break;
		case State::FetchOperator: // fetch operand
			fetched = bus->CpuRead(PC);

			switch(instruction.addrMode) {
				case IMP:
					#ifdef printDebug
					switch (instruction.instruction) {
						case Instructions::ASL:
						case Instructions::ROL:
						case Instructions::LSR:
						case Instructions::ROR:
							instrStr[strPos] = 'A';
							strPos++;
							break;
					}
					#endif
					state = State::FetchOpcode;

					switch(instruction.instruction) {
						case Instructions::JSR:
						case Instructions::BRK:
							PC++;
						case Instructions::NMI:
						case Instructions::IRQ:
						case Instructions::PHP:
						case Instructions::PLP:
						case Instructions::RTI:
						case Instructions::RTS:
						case Instructions::PHA:
						case Instructions::PLA:
							addr_abs = fetched;
							state = State::StackShit1;
							break;
						case Instructions::ASL:
							Status.C = A & 0x80;

							A <<= 1;
							Status.Z = A == 0;
							Status.N = A & 0x80;
							break;
						case Instructions::ROL:
							A = ROL(A);
							break;
						case Instructions::LSR:
							Status.C = A & 1;
						
							A >>= 1;
							Status.Z = A == 0;
							Status.N = A & 0x80;
							break;
						case Instructions::ROR:
							A = ROR(A);
							break;
						case Instructions::TXA:
							A = X;

							Status.Z = A == 0;
							Status.N = A & 0x80;
							break;
						case Instructions::TYA:
							A = Y;

							Status.Z = A == 0;
							Status.N = A & 0x80;
							break;
						case Instructions::TXS:
							SP = X;
							break;
						case Instructions::TAY:
							Y = A;

							Status.Z = Y == 0;
							Status.N = Y & 0x80;
							break;
						case Instructions::TAX:
							X = A;

							Status.Z = X == 0;
							Status.N = X & 0x80;
							break;
						case Instructions::TSX:
							X = SP;

							Status.Z = X == 0;
							Status.N = X & 0x80;
							break;
						case Instructions::DEX:
							X--;
							Status.Z = X == 0x00;
							Status.N = X & 0x80;
							break;
						case Instructions::DEY:
							Y--;
							Status.Z = Y == 0x00;
							Status.N = Y & 0x80;
							break;
						case Instructions::INX:
							X++;
							Status.Z = X == 0x00;
							Status.N = X & 0x80;
							break;
						case Instructions::INY:
							Y++;
							Status.Z = Y == 0x00;
							Status.N = Y & 0x80;
							break;
						case Instructions::SEC: Status.C = true;
							break;
						case Instructions::SED: Status.D = true;
							break;
						case Instructions::SEI: Status.I = true;
							break;
						case Instructions::CLC: Status.C = false;
							break;
						case Instructions::CLD: Status.D = false;
							break;
						case Instructions::CLI: Status.I = false;
							break;
						case Instructions::CLV: Status.V = false;
							break;
						case Instructions::NOP: break;
					}
					break;
				case IMM:
					PC++;
					#ifdef printDebug
					strPos += sprintf_s((instrStr + strPos), 27 - strPos, "#$%02X ", fetched);
					#endif
					switch(instruction.instruction) {
						case Instructions::ORA:
							A |= fetched;
							Status.Z = A == 0;
							Status.N = A & 0x80;
							break;
						case Instructions::ANC:
							A &= fetched;

							Status.Z = A == 0;
							Status.C = Status.N = A & 0x80;
							break;
						case Instructions::AND:
							A &= fetched;

							Status.Z = A == 0;
							Status.N = A & 0x80;
							break;
						case Instructions::EOR:
							A ^= fetched;

							Status.Z = A == 0;
							Status.N = A & 0x80;
							break;
						case Instructions::ALR:
							A &= fetched;
							Status.C = A & 1;

							// LSR A
							A >>= 1;

							Status.Z = A == 0;
							Status.N = A & 0x80;
							break;
						case Instructions::ADC:
							ADC(fetched);
							break;
						case Instructions::LDA:
							A = fetched;
							Status.Z = A == 0;
							Status.N = A & 0x80;
							break;
						case Instructions::LDX:
							X = fetched;
							Status.Z = X == 0;
							Status.N = X & 0x80;
							break;
						case Instructions::LDY:
							Y = fetched;
							Status.Z = Y == 0;
							Status.N = Y & 0x80;
							break;
						case Instructions::LAX:
							X = A = fetched;

							Status.Z = X == 0;
							Status.N = X & 0x80;
							break;
						case Instructions::CMP:
							Status.C = A >= fetched;
							Status.Z = A == fetched;
							Status.N = (A - fetched) & 0x80;
							break;
						case Instructions::CPX:
							Status.C = X >= fetched;
							Status.Z = X == fetched;
							Status.N = (X - fetched) & 0x80;
							break;
						case Instructions::CPY:
							Status.C = Y >= fetched;
							Status.Z = Y == fetched;
							Status.N = (Y - fetched) & 0x80;
							break;
						case Instructions::AXS:
							Status.C = (A & X) >= fetched;
							X = (A & X) - fetched;
							Status.Z = X == 0;
							Status.N = X & 0x80;
							break;
						case Instructions::SBC: SBC(fetched);
							break;
						case Instructions::ARR:
							A &= fetched;

							A = (Status.C << 7) | (A >> 1);

							Status.C = A & 0x40;
							Status.V = (A & 0x40) ^ ((A & 0x20) << 1);

							Status.Z = A == 0;
							Status.N = A & 0x80;
							break;
						case Instructions::XAA:
							A = (A | 0xFF) & X & fetched;

							Status.Z = A == 0;
							Status.N = A & 0x80;
							break;
						case Instructions::NOP: break;
					}
					state = State::FetchOpcode;
					break;
				case ZP0:
					PC++;
					addr_abs = fetched;
					state = InstructionType(instruction.instruction);
					break;
				case ZPX:
				case ZPY:
				case IZX:
					PC++;
					addr_abs = fetched;
					state = State::WeirdRead;
					break;
				case REL: {
					PC++;
					bool take = false;
					switch(instruction.instruction) {
						case Instructions::BPL: take = !Status.N;
							break;
						case Instructions::BMI: take = Status.N;
							break;
						case Instructions::BVC: take = !Status.V;
							break;
						case Instructions::BVS: take = Status.V;
							break;
						case Instructions::BCC: take = !Status.C;
							break;
						case Instructions::BCS: take = Status.C;
							break;
						case Instructions::BNE: take = !Status.Z;
							break;
						case Instructions::BEQ: take = Status.Z;
							break;
					}

					if(take) {
						addr_abs = fetched; // use addr_abs as temporary
						state = State::ReadPC;
					} else {
						state = State::FetchOpcode;
					}
					break;
				}
				case ABS:
				case ABX:
				case ABY:
				case IND:
					PC++;
					addr_abs = fetched;
					state = State::ReadPC;
					break;
				case IZY:
					PC++;
					ptr = fetched;
					state = State::ReadAddrLo;
					break;
			}
			break;
		case State::WeirdRead:
			bus->CpuRead(addr_abs);

			switch(instruction.addrMode) {
				case IZX:
					ptr = (addr_abs + X) & 0xFF;
					state = State::ReadAddrLo;
					break;
				case ZPX:
					addr_abs = (addr_abs + X) & 0xFF;
					state = InstructionType(instruction.instruction);
					break;
				case ZPY:
					addr_abs = (addr_abs + Y) & 0xFF;
					state = InstructionType(instruction.instruction);
					break;
			}
			break;
		case State::ReadPC:
			fetched = bus->CpuRead(PC);

			switch(instruction.addrMode) {
				case REL:
					if(addr_abs & 0x80) {
						// Negative 
						addr_abs |= 0xFF00;
					}
					#ifdef printDebug
					strPos += sprintf_s((instrStr + strPos), 27 - strPos, "$%04X", PC + addr_abs);
					#endif
					if(((PC + addr_abs) & 0xFF00) != (PC & 0xFF00)) {
						state = State::PageError; // page boundary
					} else {
						PC += addr_abs;
						state = State::FetchOpcode;
					}
					break;
				case ABS:
					PC++;
					addr_abs |= fetched << 8;
					switch(instruction.instruction) {
						case Instructions::JMP:
							PC = addr_abs;
							#ifdef printDebug
							strPos += sprintf_s((instrStr + strPos), 27 - strPos, "$%04X", addr_abs);
							#endif
							state = State::FetchOpcode;
							break;
						default:
							state = InstructionType(instruction.instruction);
							break;
					}
					break;
				case ABX:
					PC++;
					addr_abs |= fetched << 8;

					switch(instruction.instruction) {
						case Instructions::LDA:
						case Instructions::LDX:
						case Instructions::LDY:
						case Instructions::EOR:
						case Instructions::AND:
						case Instructions::ORA:
						case Instructions::ADC:
						case Instructions::SBC:
						case Instructions::CMP:
						case Instructions::CPX:
						case Instructions::CPY:
						case Instructions::BIT:
						case Instructions::LAX:
						case Instructions::LAS:
						case Instructions::TAS:
						case Instructions::NOP:
							if(((addr_abs + X) & 0xFF00) == (addr_abs & 0xFF00)) {
								addr_abs += X;
								state = InstructionType(instruction.instruction);
								break;
							}
						default:
							state = State::PageError;
							break;
					}
					break;
				case ABY:
					PC++;
					addr_abs |= fetched << 8;

					switch(instruction.instruction) {
						case Instructions::LDA:
						case Instructions::LDX:
						case Instructions::LDY:
						case Instructions::EOR:
						case Instructions::AND:
						case Instructions::ORA:
						case Instructions::ADC:
						case Instructions::SBC:
						case Instructions::CMP:
						case Instructions::CPX:
						case Instructions::CPY:
						case Instructions::BIT:
						case Instructions::LAX:
						case Instructions::LAS:
						case Instructions::TAS:
						case Instructions::NOP:
							if(((addr_abs + Y) & 0xFF00) == (addr_abs & 0xFF00)) {
								addr_abs += Y;
								state = InstructionType(instruction.instruction);
								break;
							}
						default:
							state = State::PageError;
							break;
					}
					break;
				case IND:
					ptr = addr_abs | (fetched << 8);
					state = State::ReadAddrLo;
					break;
			}

			break;
		case State::ReadAddrLo:
			addr_abs = bus->CpuRead(ptr);
			state = State::ReadAddrHi;
			break;
		case State::ReadAddrHi:
			addr_abs |= bus->CpuRead((ptr & 0xFF00) | ((ptr + 1) & 0xFF)) << 8;

			switch(instruction.addrMode) {
				case IZX:
					state = InstructionType(instruction.instruction);
					break;
				case IZY: {
					switch(instruction.instruction) {
						case Instructions::LDA:
						case Instructions::LDX:
						case Instructions::LDY:
						case Instructions::EOR:
						case Instructions::AND:
						case Instructions::ORA:
						case Instructions::ADC:
						case Instructions::SBC:
						case Instructions::CMP:
						case Instructions::CPX:
						case Instructions::CPY:
						case Instructions::BIT:
						case Instructions::LAX:
						case Instructions::LAS:
						case Instructions::TAS:
						case Instructions::NOP:
							if(((addr_abs + Y) & 0xFF00) == (addr_abs & 0xFF00)) {
								addr_abs += Y;
								state = InstructionType(instruction.instruction);
								break;
							}
						default:
							state = State::PageError;
							break;
					}
					break;
				}
				case IND: // JMP
					PC = addr_abs;
					#ifdef printDebug
					strPos += sprintf_s((instrStr + strPos), 27 - strPos, "$%04X", addr_abs);
					#endif
					state = State::FetchOpcode;
					break;
			}
			break;
		case State::ReadInstr:
			fetched = bus->CpuRead(addr_abs);
			#ifdef printDebug
			switch(instruction.instruction) {
				case Instructions::ASL:
				case Instructions::LSR:
				case Instructions::ROL:
				case Instructions::ROR:
				case Instructions::INC:
				case Instructions::DEC:
				case Instructions::SLO:
				case Instructions::SRE:
				case Instructions::RLA:
				case Instructions::RRA:
				case Instructions::ISC:
				case Instructions::DCP:
					break;
				default:
					switch(instruction.addrMode) {
						case ZP0:
							strPos += sprintf_s((instrStr + strPos), 27 - strPos, "$%02X", addr_abs);
							break;
						default:
							strPos += sprintf_s((instrStr + strPos), 27 - strPos, "$%04X", addr_abs);
							break;
					}
					break;
			}
			#endif

			switch(instruction.instruction) {
				case Instructions::LDA:
					A = fetched;
					Status.Z = A == 0;
					Status.N = A & 0x80;
					state = State::FetchOpcode;
					break;
				case Instructions::LDX:
					X = fetched;
					Status.Z = X == 0;
					Status.N = X & 0x80;
					state = State::FetchOpcode;
					break;
				case Instructions::LDY:
					Y = fetched;
					Status.Z = Y == 0;
					Status.N = Y & 0x80;
					state = State::FetchOpcode;
					break;
				case Instructions::EOR:
					A ^= fetched;

					Status.Z = A == 0;
					Status.N = A & 0x80;
					state = State::FetchOpcode;
					break;
				case Instructions::AND:
					A &= fetched;

					Status.Z = A == 0;
					Status.N = A & 0x80;
					state = State::FetchOpcode;
					break;
				case Instructions::ORA:
					A |= fetched;
					Status.Z = A == 0;
					Status.N = A & 0x80;
					state = State::FetchOpcode;
					break;
				case Instructions::ADC:
					ADC(fetched);
					state = State::FetchOpcode;
					break;
				case Instructions::SBC:
					SBC(fetched);
					state = State::FetchOpcode;
					break;
				case Instructions::CMP:
					Status.C = A >= fetched;
					Status.Z = A == fetched;
					Status.N = (A - fetched) & 0x80;
					state = State::FetchOpcode;
					break;
				case Instructions::CPX:
					Status.C = X >= fetched;
					Status.Z = X == fetched;
					Status.N = (X - fetched) & 0x80;
					state = State::FetchOpcode;
					break;
				case Instructions::CPY:
					Status.C = Y >= fetched;
					Status.Z = Y == fetched;
					Status.N = (Y - fetched) & 0x80;
					state = State::FetchOpcode;
					break;
				case Instructions::BIT:
					Status.Z = (A & fetched) == 0;
					Status.N = fetched & (1 << 7);
					Status.V = fetched & (1 << 6);
					state = State::FetchOpcode;
					break;
				case Instructions::LAX:
					X = A = fetched;

					Status.Z = X == 0;
					Status.N = X & 0x80;
					state = State::FetchOpcode;
					break;
				case Instructions::NOP:
					state = State::FetchOpcode;
					break;
				case Instructions::ASL:
				case Instructions::LSR:
				case Instructions::ROL:
				case Instructions::ROR:
				case Instructions::INC:
				case Instructions::DEC:
				case Instructions::SLO:
				case Instructions::SRE:
				case Instructions::RLA:
				case Instructions::RRA:
				case Instructions::ISC:
				case Instructions::DCP:
					ptr = fetched; // use ptr as temporary
					state = State::DummyWrite;
					break;
				default:
					// printf("%s not implemented\n", instruction.name.c_str());
					break;
			}

			break;
		case State::DummyWrite:
			bus->CpuWrite(addr_abs, ptr);
			state = State::WriteInstr;
			break;
		case State::WriteInstr: {
			uint8_t toWrite;
			uint16_t writeAddr = addr_abs;

			switch(instruction.instruction) {
				case Instructions::STA:
					toWrite = A;
					break;
				case Instructions::STX:
					toWrite = X;
					break;
				case Instructions::STY:
					toWrite = Y;
					break;
				case Instructions::SAX:
					toWrite = A & X;
					break;
				case Instructions::ASL:
					Status.C = ptr & 0x80;

					toWrite = ptr << 1;
					Status.Z = toWrite == 0;
					Status.N = toWrite & 0x80;
					break;
				case Instructions::LSR:
					Status.C = ptr & 1;
					toWrite = ptr >> 1;

					Status.Z = toWrite == 0;
					Status.N = toWrite & 0x80;
					break;
				case Instructions::ROL:
					toWrite = ROL(ptr);
					break;
				case Instructions::ROR:
					toWrite = ROR(ptr);
					break;
				case Instructions::INC:
					ptr++;

					toWrite = ptr;
					Status.Z = (ptr & 0xFF) == 0;
					Status.N = ptr & 0x80;
					break;
				case Instructions::DEC:
					ptr--;
					toWrite = ptr;
					Status.Z = ptr == 0;
					Status.N = ptr & 0x80;
					break;
				case Instructions::SLO:
					ptr <<= 1;
					Status.C = ptr & 0xFF00;
					toWrite = ptr;

					A |= ptr;
					Status.Z = A == 0;
					Status.N = A & 0x80;
					break;
				case Instructions::SRE:
					Status.C = ptr & 1;
					ptr >>= 1;
					toWrite = ptr;

					A ^= ptr;
					Status.Z = A == 0;
					Status.N = A & 0x80;
					break;
				case Instructions::RLA:
					ptr = (ptr << 1) | Status.C;
					Status.C = ptr & 0xFF00;
					toWrite = ptr;

					A &= ptr;
					Status.Z = A == 0;
					Status.N = A & 0x80;
					break;
				case Instructions::RRA:
					fetched = (Status.C << 7) | (ptr >> 1);
					Status.C = ptr & 1;
					toWrite = fetched;

					ptr = A + fetched + Status.C;

					Status.C = ptr > 255;
					Status.V = (~(A ^ fetched) & (A ^ ptr)) & 0x0080;

					A = ptr;

					Status.Z = A == 0;
					Status.N = A & 0x80;
					break;
				case Instructions::ISC:
					ptr++;
					toWrite = ptr;

					fetched = ptr ^ 0xFF;
					ptr = A + fetched + Status.C;

					Status.C = ptr > 255;
					Status.V = (ptr ^ A) & (ptr ^ fetched) & 0x0080;

					A = ptr;

					Status.Z = A == 0;
					Status.N = A & 0x80;
					break;
				case Instructions::DCP:
					ptr--;
					toWrite = ptr;

					Status.C = A >= (ptr & 0xFF);
					ptr = A - ptr;
					Status.Z = (ptr & 0x00FF) == 0;
					Status.N = ptr & 0x80;
					break;
				case Instructions::SHX:
					ptr = ((addr_abs - Y) & 0xFF00) | (addr_abs & 0xFF);

					if(ptr >> 8 != addr_abs >> 8) {
						ptr &= X << 8;
					}
					writeAddr = ptr;
					toWrite = X & ((ptr >> 8) + 1);
					break;
				case Instructions::SHY:
					ptr = ((addr_abs - X) & 0xFF00) | (addr_abs & 0xFF);

					if(ptr >> 8 != addr_abs >> 8) {
						ptr &= Y << 8;
					}
					writeAddr = ptr;
					toWrite = Y & ((ptr >> 8) + 1);
					break;
				case Instructions::TAS:
					SP = A & X; // IDK
				case Instructions::AHX:
					ptr = ((addr_abs - Y) & 0xFF00) | (addr_abs & 0xFF);

					if(ptr >> 8 != addr_abs >> 8) {
						ptr &= (X & A) << 8;
					}
					writeAddr = ptr;
					toWrite = X & A & ((ptr >> 8) + 1);
					break;
				default: throw std::logic_error("Not reachable");
			}
			#ifdef printDebug
			switch(instruction.addrMode) {
				case ZP0:
					strPos += sprintf_s((instrStr + strPos), 27 - strPos, "$%02X", addr_abs);
					break;
				default:
					strPos += sprintf_s((instrStr + strPos), 27 - strPos, "$%04X", addr_abs);
					break;
			}
			#endif
			bus->CpuWrite(writeAddr, toWrite);
		}
			state = State::FetchOpcode;
			break;
		case State::StackShit1:
			switch(instruction.instruction) {
				case Instructions::BRK:
				case Instructions::NMI:
				case Instructions::IRQ:
					PushStack(PC >> 8);
					state = State::StackShit2;
					break;
				case Instructions::RTI:
				case Instructions::PLA:
				case Instructions::PLP:
				case Instructions::RTS:
				case Instructions::JSR:
					bus->CpuRead(0x0100 + SP);
					state = State::StackShit2;
					break;
				case Instructions::PHA:
					PushStack(A);
					state = State::FetchOpcode;
					break;
				case Instructions::PHP:
					Status.B = true;
					PushStack(Status.reg);
					Status.B = false;
					state = State::FetchOpcode;
					break;
			}
			break;
		case State::StackShit2:
			switch(instruction.instruction) {
				case Instructions::BRK:
				case Instructions::NMI:
				case Instructions::IRQ:
					PushStack(PC);
					state = State::StackShit3;
					break;
				case Instructions::RTI:
					Status.reg = PopStack();
					Status.B = false;
					Status.U = true;
					state = State::StackShit3;
					break;
				case Instructions::PLA:
					A = PopStack();

					Status.Z = A == 0;
					Status.N = A & 0x80;
					state = State::FetchOpcode;
					break;
				case Instructions::PLP:
					Status.reg = PopStack();
					Status.B = false;
					Status.U = true;
					state = State::FetchOpcode;
					break;
				case Instructions::RTS:
					PC = PopStack();
					state = State::StackShit3;
					break;
				case Instructions::JSR:
					PushStack(PC >> 8);
					state = State::StackShit3;
					break;
			}
			break;
		case State::StackShit3:
			switch(instruction.instruction) {
				case Instructions::BRK:
					Status.B = true;
				case Instructions::IRQ:
					if(NMI) {
						goto nmi;
					}
					addr_abs = 0xFFFE;
					Status.U = true;
					PushStack(Status.reg);
					Status.I = true;
					break;
				case Instructions::NMI:
				nmi:
					NMI = false;
					addr_abs = 0xFFFA;
					PushStack(Status.reg);
					break;
				case Instructions::RTI:
					PC = PopStack();
					break;
				case Instructions::RTS:
					PC |= PopStack() << 8;
					break;
				case Instructions::JSR:
					PushStack(PC);
					break;
			}
			state = State::StackShit4;
			break;
		case State::StackShit4:
			state = State::FetchOpcode;

			switch(instruction.instruction) {
				case Instructions::BRK:
				case Instructions::IRQ:
				case Instructions::NMI:
					PC = bus->CpuRead(addr_abs);
					state = State::StackShit5;
					break;
				case Instructions::RTI:
					PC |= PopStack() << 8;
					Status.B = false;
					Status.U = true;
					break;
				case Instructions::RTS:
					bus->CpuRead(PC);
					PC++;
					break;
				case Instructions::JSR:
					PC = addr_abs | (bus->CpuRead(PC) << 8);
					#ifdef printDebug
					strPos += sprintf_s((instrStr + strPos), 27 - strPos, "$%04X", PC);
					#endif
					break;
			}
			break;
		case State::StackShit5:
			PC |= bus->CpuRead(addr_abs + 1) << 8;
			state = State::FetchOpcode;
			break;
		case State::PageError:
			switch(instruction.addrMode) {
				case REL:
					bus->CpuRead((PC & 0xFF00) | ((PC + addr_abs) & 0xFF));
					PC += addr_abs;
					state = State::FetchOpcode;
					break;
				case ABX:
					bus->CpuRead((addr_abs & 0xFF00) | ((addr_abs + X) & 0xFF));
					addr_abs += X;
					state = InstructionType(instruction.instruction);
					break;
				case ABY:
				case IZY:
					bus->CpuRead((addr_abs & 0xFF00) | ((addr_abs + Y) & 0xFF));
					addr_abs += Y;
					state = InstructionType(instruction.instruction);
					break;
			}
			break;
	}

	// cycles++;
}

void mos6502::PushStack(uint8_t val) {
	bus->CpuWrite(0x0100 + SP, val);
	SP--;
}

uint8_t mos6502::PopStack() {
	SP++;
	return bus->CpuRead(0x0100 + SP);
}

void mos6502::ADC(uint8_t val) {
	uint16_t temp = A + val + Status.C;

	Status.C = temp > 255;
	Status.V = ~(A ^ val) & (A ^ temp) & 0x80;

	A = temp;

	Status.Z = A == 0;
	Status.N = A & 0x80;
}

void mos6502::SBC(uint8_t val) {
	val ^= 0xFF;
	uint16_t temp = A + val + Status.C;

	Status.C = temp & 0xFF00;
	Status.V = (temp ^ A) & (temp ^ val) & 0x0080;

	A = temp;

	Status.Z = A == 0;
	Status.N = A & 0x80;
}

uint8_t mos6502::ROL(uint8_t val) {
	uint8_t tmp = (val << 1) | Status.C;

	Status.C = val & 0x80;
	Status.Z = tmp == 0;
	Status.N = tmp & 0x80;

	return tmp;
}

uint8_t mos6502::ROR(uint8_t val) {
	uint8_t tmp = (Status.C << 7) | (val >> 1);
	Status.C = val & 1;

	Status.Z = tmp == 0;
	Status.N = tmp & 0x80;

	return tmp;
}
