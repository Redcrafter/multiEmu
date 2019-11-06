#include "mos6502.h"
#include "Bus.h"

// address mode of jsr changed
const Instruction lookup[] = {
	{"BRK", Instructions::BRK, IMP}, {"ORA", Instructions::ORA, IZX}, {"kil", Instructions::KIL, IMM}, {"slo", Instructions::SLO, IZX}, {"nop", Instructions::NOP, ZP0}, {"ORA", Instructions::ORA, ZP0}, {"ASL", Instructions::ASL, ZP0}, {"slo", Instructions::SLO, ZP0}, {"PHP", Instructions::PHP, IMP}, {"ORA", Instructions::ORA, IMM}, {"ASL", Instructions::ASL, IMP}, {"anc", Instructions::ANC, IMM}, {"nop", Instructions::NOP, ABS}, {"ORA", Instructions::ORA, ABS}, {"ASL", Instructions::ASL, ABS}, {"slo", Instructions::SLO, ABS},
	{"BPL", Instructions::BPL, REL}, {"ORA", Instructions::ORA, IZY}, {"kil", Instructions::KIL, IMM}, {"slo", Instructions::SLO, IZY}, {"nop", Instructions::NOP, ZPX}, {"ORA", Instructions::ORA, ZPX}, {"ASL", Instructions::ASL, ZPX}, {"slo", Instructions::SLO, ZPX}, {"CLC", Instructions::CLC, IMP}, {"ORA", Instructions::ORA, ABY}, {"nop", Instructions::NOP, IMP}, {"slo", Instructions::SLO, ABY}, {"nop", Instructions::NOP, ABX}, {"ORA", Instructions::ORA, ABX}, {"ASL", Instructions::ASL, ABX}, {"slo", Instructions::SLO, ABX},
	{"JSR", Instructions::JSR, IMP}, {"AND", Instructions::AND, IZX}, {"kil", Instructions::KIL, IMM}, {"rla", Instructions::RLA, IZX}, {"BIT", Instructions::BIT, ZP0}, {"AND", Instructions::AND, ZP0}, {"ROL", Instructions::ROL, ZP0}, {"rla", Instructions::RLA, ZP0}, {"PLP", Instructions::PLP, IMP}, {"AND", Instructions::AND, IMM}, {"ROL", Instructions::ROL, IMP}, {"anc", Instructions::ANC, IMM}, {"BIT", Instructions::BIT, ABS}, {"AND", Instructions::AND, ABS}, {"ROL", Instructions::ROL, ABS}, {"rla", Instructions::RLA, ABS},
	{"BMI", Instructions::BMI, REL}, {"AND", Instructions::AND, IZY}, {"kil", Instructions::KIL, IMM}, {"rla", Instructions::RLA, IZY}, {"nop", Instructions::NOP, ZPX}, {"AND", Instructions::AND, ZPX}, {"ROL", Instructions::ROL, ZPX}, {"rla", Instructions::RLA, ZPX}, {"SEC", Instructions::SEC, IMP}, {"AND", Instructions::AND, ABY}, {"nop", Instructions::NOP, IMP}, {"rla", Instructions::RLA, ABY}, {"nop", Instructions::NOP, ABX}, {"AND", Instructions::AND, ABX}, {"ROL", Instructions::ROL, ABX}, {"rla", Instructions::RLA, ABX},
	{"RTI", Instructions::RTI, IMP}, {"EOR", Instructions::EOR, IZX}, {"kil", Instructions::KIL, IMM}, {"sre", Instructions::SRE, IZX}, {"nop", Instructions::NOP, ZP0}, {"EOR", Instructions::EOR, ZP0}, {"LSR", Instructions::LSR, ZP0}, {"sre", Instructions::SRE, ZP0}, {"PHA", Instructions::PHA, IMP}, {"EOR", Instructions::EOR, IMM}, {"LSR", Instructions::LSR, IMP}, {"alr", Instructions::ALR, IMM}, {"JMP", Instructions::JMP, ABS}, {"EOR", Instructions::EOR, ABS}, {"LSR", Instructions::LSR, ABS}, {"sre", Instructions::SRE, ABS},
	{"BVC", Instructions::BVC, REL}, {"EOR", Instructions::EOR, IZY}, {"kil", Instructions::KIL, IMM}, {"sre", Instructions::SRE, IZY}, {"nop", Instructions::NOP, ZPX}, {"EOR", Instructions::EOR, ZPX}, {"LSR", Instructions::LSR, ZPX}, {"sre", Instructions::SRE, ZPX}, {"CLI", Instructions::CLI, IMP}, {"EOR", Instructions::EOR, ABY}, {"nop", Instructions::NOP, IMP}, {"sre", Instructions::SRE, ABY}, {"nop", Instructions::NOP, ABX}, {"EOR", Instructions::EOR, ABX}, {"LSR", Instructions::LSR, ABX}, {"sre", Instructions::SRE, ABX},
	{"RTS", Instructions::RTS, IMP}, {"ADC", Instructions::ADC, IZX}, {"kil", Instructions::KIL, IMM}, {"rra", Instructions::RRA, IZX}, {"nop", Instructions::NOP, ZP0}, {"ADC", Instructions::ADC, ZP0}, {"ROR", Instructions::ROR, ZP0}, {"rra", Instructions::RRA, ZP0}, {"PLA", Instructions::PLA, IMP}, {"ADC", Instructions::ADC, IMM}, {"ROR", Instructions::ROR, IMP}, {"arr", Instructions::ARR, IMM}, {"JMP", Instructions::JMP, IND}, {"ADC", Instructions::ADC, ABS}, {"ROR", Instructions::ROR, ABS}, {"rra", Instructions::RRA, ABS},
	{"BVS", Instructions::BVS, REL}, {"ADC", Instructions::ADC, IZY}, {"kil", Instructions::KIL, IMM}, {"rra", Instructions::RRA, IZY}, {"nop", Instructions::NOP, ZPX}, {"ADC", Instructions::ADC, ZPX}, {"ROR", Instructions::ROR, ZPX}, {"rra", Instructions::RRA, ZPX}, {"SEI", Instructions::SEI, IMP}, {"ADC", Instructions::ADC, ABY}, {"nop", Instructions::NOP, IMP}, {"rra", Instructions::RRA, ABY}, {"nop", Instructions::NOP, ABX}, {"ADC", Instructions::ADC, ABX}, {"ROR", Instructions::ROR, ABX}, {"rra", Instructions::RRA, ABX},
	{"nop", Instructions::NOP, IMM}, {"STA", Instructions::STA, IZX}, {"nop", Instructions::NOP, IMM}, {"sax", Instructions::SAX, IZX}, {"STY", Instructions::STY, ZP0}, {"STA", Instructions::STA, ZP0}, {"STX", Instructions::STX, ZP0}, {"sax", Instructions::SAX, ZP0}, {"DEY", Instructions::DEY, IMP}, {"nop", Instructions::NOP, IMM}, {"TXA", Instructions::TXA, IMP}, {"xaa", Instructions::XAA, IMM}, {"STY", Instructions::STY, ABS}, {"STA", Instructions::STA, ABS}, {"STX", Instructions::STX, ABS}, {"sax", Instructions::SAX, ABS},
	{"BCC", Instructions::BCC, REL}, {"STA", Instructions::STA, IZY}, {"kil", Instructions::KIL, IMM}, {"ahx", Instructions::AHX, IZY}, {"STY", Instructions::STY, ZPX}, {"STA", Instructions::STA, ZPX}, {"STX", Instructions::STX, ZPY}, {"sax", Instructions::SAX, ZPY}, {"TYA", Instructions::TYA, IMP}, {"STA", Instructions::STA, ABY}, {"TXS", Instructions::TXS, IMP}, {"tas", Instructions::TAS, ABY}, {"shy", Instructions::SHY, ABX}, {"STA", Instructions::STA, ABX}, {"shx", Instructions::SHX, ABY}, {"ahx", Instructions::AHX, ABY},
	{"LDY", Instructions::LDY, IMM}, {"LDA", Instructions::LDA, IZX}, {"LDX", Instructions::LDX, IMM}, {"lax", Instructions::LAX, IZX}, {"LDY", Instructions::LDY, ZP0}, {"LDA", Instructions::LDA, ZP0}, {"LDX", Instructions::LDX, ZP0}, {"lax", Instructions::LAX, ZP0}, {"TAY", Instructions::TAY, IMP}, {"LDA", Instructions::LDA, IMM}, {"TAX", Instructions::TAX, IMP}, {"lax", Instructions::LAX, IMM}, {"LDY", Instructions::LDY, ABS}, {"LDA", Instructions::LDA, ABS}, {"LDX", Instructions::LDX, ABS}, {"lax", Instructions::LAX, ABS},
	{"BCS", Instructions::BCS, REL}, {"LDA", Instructions::LDA, IZY}, {"kil", Instructions::KIL, IMM}, {"lax", Instructions::LAX, IZY}, {"LDY", Instructions::LDY, ZPX}, {"LDA", Instructions::LDA, ZPX}, {"LDX", Instructions::LDX, ZPY}, {"lax", Instructions::LAX, ZPY}, {"CLV", Instructions::CLV, IMP}, {"LDA", Instructions::LDA, ABY}, {"TSX", Instructions::TSX, IMP}, {"las", Instructions::LAS, ABY}, {"LDY", Instructions::LDY, ABX}, {"LDA", Instructions::LDA, ABX}, {"LDX", Instructions::LDX, ABY}, {"lax", Instructions::LAX, ABY},
	{"CPY", Instructions::CPY, IMM}, {"CMP", Instructions::CMP, IZX}, {"nop", Instructions::NOP, IMM}, {"dcp", Instructions::DCP, IZX}, {"CPY", Instructions::CPY, ZP0}, {"CMP", Instructions::CMP, ZP0}, {"DEC", Instructions::DEC, ZP0}, {"dcp", Instructions::DCP, ZP0}, {"INY", Instructions::INY, IMP}, {"CMP", Instructions::CMP, IMM}, {"DEX", Instructions::DEX, IMP}, {"axs", Instructions::AXS, IMM}, {"CPY", Instructions::CPY, ABS}, {"CMP", Instructions::CMP, ABS}, {"DEC", Instructions::DEC, ABS}, {"dcp", Instructions::DCP, ABS},
	{"BNE", Instructions::BNE, REL}, {"CMP", Instructions::CMP, IZY}, {"kil", Instructions::KIL, IMM}, {"dcp", Instructions::DCP, IZY}, {"nop", Instructions::NOP, ZPX}, {"CMP", Instructions::CMP, ZPX}, {"DEC", Instructions::DEC, ZPX}, {"dcp", Instructions::DCP, ZPX}, {"CLD", Instructions::CLD, IMP}, {"CMP", Instructions::CMP, ABY}, {"nop", Instructions::NOP, IMP}, {"dcp", Instructions::DCP, ABY}, {"nop", Instructions::NOP, ABX}, {"CMP", Instructions::CMP, ABX}, {"DEC", Instructions::DEC, ABX}, {"dcp", Instructions::DCP, ABX},
	{"CPX", Instructions::CPX, IMM}, {"SBC", Instructions::SBC, IZX}, {"nop", Instructions::NOP, IMM}, {"isc", Instructions::ISC, IZX}, {"CPX", Instructions::CPX, ZP0}, {"SBC", Instructions::SBC, ZP0}, {"INC", Instructions::INC, ZP0}, {"isc", Instructions::ISC, ZP0}, {"INX", Instructions::INX, IMP}, {"SBC", Instructions::SBC, IMM}, {"NOP", Instructions::NOP, IMP}, {"sbc", Instructions::SBC, IMM}, {"CPX", Instructions::CPX, ABS}, {"SBC", Instructions::SBC, ABS}, {"INC", Instructions::INC, ABS}, {"isc", Instructions::ISC, ABS},
	{"BEQ", Instructions::BEQ, REL}, {"SBC", Instructions::SBC, IZY}, {"kil", Instructions::KIL, IMM}, {"isc", Instructions::ISC, IZY}, {"nop", Instructions::NOP, ZPX}, {"SBC", Instructions::SBC, ZPX}, {"INC", Instructions::INC, ZPX}, {"isc", Instructions::ISC, ZPX}, {"SED", Instructions::SED, IMP}, {"SBC", Instructions::SBC, ABY}, {"nop", Instructions::NOP, IMP}, {"isc", Instructions::ISC, ABY}, {"nop", Instructions::NOP, ABX}, {"SBC", Instructions::SBC, ABX}, {"INC", Instructions::INC, ABX}, {"isc", Instructions::ISC, ABX},
};

cpu6502::cpu6502(Bus* bus): bus(bus) {
	PC = bus->CpuRead(0xFFFC) | (bus->CpuRead(0xFFFD) << 8);
	
	#ifdef printDebug
	file.open("D:\\Daten\\Desktop\\test.log");
	#endif
}

cpu6502::~cpu6502() {
	#ifdef printDebug
	file.close();
	#endif
}

void cpu6502::Reset() {
	uint16_t lo = bus->CpuRead(0xFFFC);
	uint16_t hi = bus->CpuRead(0xFFFD);

	PC = (hi << 8) | lo;
	SP -= 3;

	Status.I = Status.U = true;
	state = State::FetchOpcode;
	NMI = false;
}

void cpu6502::Nmi() {
	NMI = true;
}

void cpu6502::SaveState(saver& saver) const {
	saver << NMI;

	saver << A;
	saver << X;
	saver << Y;
	saver << SP;
	saver << PC;
	saver << Status.reg;
	saver << static_cast<uint8_t>(state);

	saver << ptr;
	saver << addr_abs;

	saver << instruction.name;
	saver << static_cast<uint8_t>(instruction.instruction);
	saver << static_cast<uint8_t>(instruction.addrMode);

	saver << cycles;
}

void cpu6502::LoadState(saver& saver) {
	uint8_t temp;

	saver >> NMI;

	saver >> A;
	saver >> X;
	saver >> Y;
	saver >> SP;
	saver >> PC;
	saver >> Status.reg;
	saver >> temp;
	state = static_cast<State>(temp);

	saver >> ptr;
	saver >> addr_abs;

	instruction.name = saver.ReadString();
	saver >> temp;
	instruction.instruction = static_cast<Instructions>(temp);
	saver >> temp;
	instruction.addrMode = static_cast<AddressingModes>(temp);

	saver >> cycles;
}

void cpu6502::Clock() {
	uint8_t fetched = 0;
	#ifdef printDebug
	static char instrStr[256] = "\0";
	static int strPos = 0;
	#endif

	switch(state) {
		case State::FetchOpcode:
			#ifdef printDebug
			if(strPos != 0) {
				instrStr[strPos] = '\n';
				instrStr[strPos + 1] = '\0';
				printf_s(instrStr);
				file << instrStr;
			}
			#endif
			if(NMI) {
				instruction.instruction = Instructions::NMI;
				instruction.addrMode = IMP;
				instruction.name = "NMI";

				#ifdef printDebug
				strPos = sprintf_s(instrStr, "%04X: %s  ", PC, instruction.name.c_str());
				#endif
			} else if(IRQ && !Status.I) {
				instruction.instruction = Instructions::IRQ;
				instruction.addrMode = IMP;
				instruction.name = "IRQ";

				#ifdef printDebug
				strPos = sprintf_s(instrStr, "%04X: %s  ", PC, instruction.name.c_str());
				#endif
			} else {
				fetched = bus->CpuRead(PC);
				instruction = lookup[fetched];

				#ifdef printDebug
				strPos = sprintf_s(instrStr, "%04X: %s  ", PC, instruction.name.c_str());
				#endif

				PC++;
			}
			state = State::FetchOperator;
			break;
		case State::FetchOperator: // fetch operand
			fetched = bus->CpuRead(PC);

			switch(instruction.addrMode) {
				case IMP:
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
						case Instructions::ASL: A = ASL(A);
							break;
						case Instructions::ROL: A = ROL(A);
							break;
						case Instructions::LSR: A = LSR(A);
							break;
						case Instructions::ROR: A = ROR(A);
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
					strPos += sprintf_s((instrStr + strPos), sizeof(instrStr) - strPos, "#$%02X", fetched);
					#endif
					switch(instruction.instruction) {
						case Instructions::ORA: ORA(fetched);
							break;
						case Instructions::ANC: ANC(fetched);
							break;
						case Instructions::AND: AND(fetched);
							break;
						case Instructions::EOR: EOR(fetched);
							break;
						case Instructions::ALR:
							A &= fetched;
							Status.C = A & 1;

							// LSR A
							A >>= 1;

							Status.Z = A == 0;
							Status.N = A & 0x80;
							break;
						case Instructions::ADC: ADC(fetched);
							break;
						case Instructions::LDA: LDA(fetched);
							break;
						case Instructions::LDX: LDX(fetched);
							break;
						case Instructions::LDY: LDY(fetched);
							break;
						case Instructions::LAX: LAX(fetched);
							break;
						case Instructions::CMP: CMP(fetched);
							break;
						case Instructions::CPX: CPX(fetched);
							break;
						case Instructions::CPY: CPY(fetched);
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
						case Instructions::XAA: printf("XAA Not implemented");
						case Instructions::NOP: break;
					}
					state = State::FetchOpcode;
					break;
				case ZP0:
					PC++;
					addr_abs = fetched;
					state = InstructionType();
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
					state = InstructionType();
					break;
				case ZPY:
					addr_abs = (addr_abs + Y) & 0xFF;
					state = InstructionType();
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
					if(addr_abs & 0x80) {
						strPos += sprintf_s((instrStr + strPos), sizeof(instrStr) - strPos, "-$%02X", (0xFFFF - addr_abs) & 0xFF);
					} else {
						strPos += sprintf_s((instrStr + strPos), sizeof(instrStr) - strPos, "$%02X", addr_abs & 0xFF);
					}
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
							strPos += sprintf_s((instrStr + strPos), sizeof(instrStr) - strPos, "$%04X", addr_abs);
							#endif
							state = State::FetchOpcode;
							break;
						default:
							state = InstructionType();
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
								state = InstructionType();
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
								state = InstructionType();
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
					state = InstructionType();
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
								state = InstructionType();
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
					strPos += sprintf_s((instrStr + strPos), sizeof(instrStr) - strPos, "$%04X = %04X", ptr, addr_abs);
					#endif
					state = State::FetchOpcode;
					break;
			}
			break;
		case State::ReadInstr:
			fetched = bus->CpuRead(addr_abs);
			#ifdef printDebug
			strPos += sprintf_s((instrStr + strPos), sizeof(instrStr) - strPos, "$%04X = %02X", addr_abs, fetched);
			#endif

			switch(instruction.instruction) {
				case Instructions::LDA:
					LDA(fetched);
					state = State::FetchOpcode;
					break;
				case Instructions::LDX:
					LDX(fetched);
					state = State::FetchOpcode;
					break;
				case Instructions::LDY:
					LDY(fetched);
					state = State::FetchOpcode;
					break;
				case Instructions::EOR:
					EOR(fetched);
					state = State::FetchOpcode;
					break;
				case Instructions::AND:
					AND(fetched);
					state = State::FetchOpcode;
					break;
				case Instructions::ORA:
					ORA(fetched);
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
					CMP(fetched);
					state = State::FetchOpcode;
					break;
				case Instructions::CPX:
					CPX(fetched);
					state = State::FetchOpcode;
					break;
				case Instructions::CPY:
					CPY(fetched);
					state = State::FetchOpcode;
					break;
				case Instructions::BIT:
					Status.Z = (A & fetched) == 0;
					Status.N = fetched & (1 << 7);
					Status.V = fetched & (1 << 6);
					state = State::FetchOpcode;
					break;
				case Instructions::LAX:
					LAX(fetched);
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
					toWrite = ASL(ptr);
					break;
				case Instructions::LSR:
					toWrite = LSR(ptr);
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
			strPos += sprintf_s((instrStr + strPos), sizeof(instrStr) - strPos, "$%04X = %02X", addr_abs, toWrite);
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
					strPos += sprintf_s((instrStr + strPos), sizeof(instrStr) - strPos, "$%04X", PC);
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
					state = InstructionType();
					break;
				case ABY:
				case IZY:
					bus->CpuRead((addr_abs & 0xFF00) | ((addr_abs + Y) & 0xFF));
					addr_abs += Y;
					state = InstructionType();
					break;
			}
			break;
	}

	cycles++;
}

void cpu6502::ORA(uint8_t val) {
	A |= val;
	Status.Z = A == 0;
	Status.N = A & 0x80;
}

State cpu6502::InstructionType() const {
	switch(instruction.instruction) {
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
		default: printf("%s\n", instruction.name.c_str());
			throw std::logic_error("whops");
	}
}

void cpu6502::PushStack(uint8_t val) {
	bus->CpuWrite(0x0100 + SP, val);
	SP--;
}

uint8_t cpu6502::PopStack() {
	SP++;
	return bus->CpuRead(0x0100 + SP);
}

void cpu6502::ANC(uint8_t val) {
	A &= val;

	Status.Z = A == 0;
	Status.C = Status.N = A & 0x80;
}

void cpu6502::AND(uint8_t val) {
	A &= val;

	Status.Z = A == 0;
	Status.N = A & 0x80;
}

void cpu6502::EOR(uint8_t val) {
	A ^= val;

	Status.Z = A == 0;
	Status.N = A & 0x80;
}

void cpu6502::ADC(uint8_t val) {
	uint16_t temp = A + val + Status.C;

	Status.C = temp > 255;
	Status.V = (~(A ^ val) & (A ^ temp)) & 0x0080;

	A = temp;

	Status.Z = A == 0;
	Status.N = A & 0x80;
}

void cpu6502::SBC(uint8_t val) {
	val ^= 0xFF;
	const uint16_t temp = A + val + Status.C;

	Status.C = temp > 255;
	Status.V = (temp ^ A) & (temp ^ val) & 0x0080;

	A = temp;

	Status.Z = A == 0;
	Status.N = A & 0x80;
}

uint8_t cpu6502::ASL(uint8_t val) {
	Status.C = val & 0x80;

	val = val << 1;
	Status.Z = val == 0;
	Status.N = val & 0x80;

	return val;
}

uint8_t cpu6502::ROL(uint8_t val) {
	uint16_t tmp = (val << 1) | Status.C;

	Status.C = tmp & 0xFF00;
	Status.Z = (tmp & 0xFF) == 0;
	Status.N = tmp & 0x80;

	return tmp;
}

uint8_t cpu6502::LSR(uint8_t val) {
	Status.C = val & 1;
	val >>= 1;

	Status.Z = val == 0;
	Status.N = val & 0x80;

	return val;
}

uint8_t cpu6502::ROR(uint8_t val) {
	uint16_t tmp = (Status.C << 7) | (val >> 1);
	Status.C = val & 1;

	Status.Z = (tmp & 0xFF) == 0;
	Status.N = tmp & 0x80;

	return tmp;
}

void cpu6502::CMP(uint8_t val) {
	Status.C = A >= val;

	val = A - val;
	Status.Z = (val & 0x00FF) == 0;
	Status.N = val & 0x80;
}

void cpu6502::CPX(uint8_t val) {
	Status.C = X >= val;

	val = X - val;
	Status.Z = (val & 0xFF) == 0;
	Status.N = val & 0x80;
}

void cpu6502::CPY(uint8_t val) {
	Status.C = Y >= val;

	val = Y - val;
	Status.Z = (val & 0xFF) == 0;
	Status.N = val & 0x80;
}

void cpu6502::LDA(uint8_t val) {
	A = val;
	Status.Z = A == 0;
	Status.N = A & 0x80;
}

void cpu6502::LDX(uint8_t val) {
	X = val;
	Status.Z = X == 0;
	Status.N = X & 0x80;
}

void cpu6502::LDY(uint8_t val) {
	Y = val;
	Status.Z = Y == 0;
	Status.N = Y & 0x80;
}

void cpu6502::LAX(uint8_t val) {
	X = A = val;

	Status.Z = X == 0;
	Status.N = X & 0x80;
}
