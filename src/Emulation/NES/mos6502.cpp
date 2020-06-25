#include "mos6502.h"
#include "Bus.h"

const char* names[77] = {
	"NMI",
	"IRQ",

	"ORA",
	"AND",
	"EOR",
	"ADC",
	"SBC",
	"CMP",
	"CPX",
	"CPY",
	"DEC",
	"DEX",
	"DEY",
	"INC",
	"INX",
	"INY",
	"ASL",
	"ROL",
	"LSR",
	"ROR",

	"LDA",
	"STA",
	"LDX",
	"STX",
	"LDY",
	"STY",
	"TAX",
	"TXA",
	"TAY",
	"TYA",
	"TSX",
	"TXS",
	"PLA",
	"PHA",
	"PLP",
	"PHP",

	"BPL",
	"BMI",
	"BVC",
	"BVS",
	"BCC",
	"BCS",
	"BNE",
	"BEQ",
	"BRK",
	"RTI",
	"JSR",
	"RTS",
	"JMP",
	"BIT",
	"CLC",
	"SEC",
	"CLD",
	"SED",
	"CLI",
	"SEI",
	"CLV",
	"NOP",

	"KIL",

	"SLO",
	"RLA",
	"SRE",
	"RRA",
	"SAX",
	"DCP",
	"ISC",
	"ANC",
	"ALR",
	"ARR",
	"XAA",
	"LAX",
	"AXS",
	"AHX",
	"SHY",
	"SHX",
	"TAS",
	"LAS"
};

// address mode of jsr changed
const Instruction lookup[] = {
	{Instructions::BRK, IMP}, {Instructions::ORA, IZX}, {Instructions::KIL, IMM}, {Instructions::SLO, IZX}, {Instructions::NOP, ZP0}, {Instructions::ORA, ZP0}, {Instructions::ASL, ZP0}, {Instructions::SLO, ZP0}, {Instructions::PHP, IMP}, {Instructions::ORA, IMM}, {Instructions::ASL, IMP}, {Instructions::ANC, IMM}, {Instructions::NOP, ABS}, {Instructions::ORA, ABS}, {Instructions::ASL, ABS}, {Instructions::SLO, ABS},
	{Instructions::BPL, REL}, {Instructions::ORA, IZY}, {Instructions::KIL, IMM}, {Instructions::SLO, IZY}, {Instructions::NOP, ZPX}, {Instructions::ORA, ZPX}, {Instructions::ASL, ZPX}, {Instructions::SLO, ZPX}, {Instructions::CLC, IMP}, {Instructions::ORA, ABY}, {Instructions::NOP, IMP}, {Instructions::SLO, ABY}, {Instructions::NOP, ABX}, {Instructions::ORA, ABX}, {Instructions::ASL, ABX}, {Instructions::SLO, ABX},
	{Instructions::JSR, IMP}, {Instructions::AND, IZX}, {Instructions::KIL, IMM}, {Instructions::RLA, IZX}, {Instructions::BIT, ZP0}, {Instructions::AND, ZP0}, {Instructions::ROL, ZP0}, {Instructions::RLA, ZP0}, {Instructions::PLP, IMP}, {Instructions::AND, IMM}, {Instructions::ROL, IMP}, {Instructions::ANC, IMM}, {Instructions::BIT, ABS}, {Instructions::AND, ABS}, {Instructions::ROL, ABS}, {Instructions::RLA, ABS},
	{Instructions::BMI, REL}, {Instructions::AND, IZY}, {Instructions::KIL, IMM}, {Instructions::RLA, IZY}, {Instructions::NOP, ZPX}, {Instructions::AND, ZPX}, {Instructions::ROL, ZPX}, {Instructions::RLA, ZPX}, {Instructions::SEC, IMP}, {Instructions::AND, ABY}, {Instructions::NOP, IMP}, {Instructions::RLA, ABY}, {Instructions::NOP, ABX}, {Instructions::AND, ABX}, {Instructions::ROL, ABX}, {Instructions::RLA, ABX},
	{Instructions::RTI, IMP}, {Instructions::EOR, IZX}, {Instructions::KIL, IMM}, {Instructions::SRE, IZX}, {Instructions::NOP, ZP0}, {Instructions::EOR, ZP0}, {Instructions::LSR, ZP0}, {Instructions::SRE, ZP0}, {Instructions::PHA, IMP}, {Instructions::EOR, IMM}, {Instructions::LSR, IMP}, {Instructions::ALR, IMM}, {Instructions::JMP, ABS}, {Instructions::EOR, ABS}, {Instructions::LSR, ABS}, {Instructions::SRE, ABS},
	{Instructions::BVC, REL}, {Instructions::EOR, IZY}, {Instructions::KIL, IMM}, {Instructions::SRE, IZY}, {Instructions::NOP, ZPX}, {Instructions::EOR, ZPX}, {Instructions::LSR, ZPX}, {Instructions::SRE, ZPX}, {Instructions::CLI, IMP}, {Instructions::EOR, ABY}, {Instructions::NOP, IMP}, {Instructions::SRE, ABY}, {Instructions::NOP, ABX}, {Instructions::EOR, ABX}, {Instructions::LSR, ABX}, {Instructions::SRE, ABX},
	{Instructions::RTS, IMP}, {Instructions::ADC, IZX}, {Instructions::KIL, IMM}, {Instructions::RRA, IZX}, {Instructions::NOP, ZP0}, {Instructions::ADC, ZP0}, {Instructions::ROR, ZP0}, {Instructions::RRA, ZP0}, {Instructions::PLA, IMP}, {Instructions::ADC, IMM}, {Instructions::ROR, IMP}, {Instructions::ARR, IMM}, {Instructions::JMP, IND}, {Instructions::ADC, ABS}, {Instructions::ROR, ABS}, {Instructions::RRA, ABS},
	{Instructions::BVS, REL}, {Instructions::ADC, IZY}, {Instructions::KIL, IMM}, {Instructions::RRA, IZY}, {Instructions::NOP, ZPX}, {Instructions::ADC, ZPX}, {Instructions::ROR, ZPX}, {Instructions::RRA, ZPX}, {Instructions::SEI, IMP}, {Instructions::ADC, ABY}, {Instructions::NOP, IMP}, {Instructions::RRA, ABY}, {Instructions::NOP, ABX}, {Instructions::ADC, ABX}, {Instructions::ROR, ABX}, {Instructions::RRA, ABX},
	{Instructions::NOP, IMM}, {Instructions::STA, IZX}, {Instructions::NOP, IMM}, {Instructions::SAX, IZX}, {Instructions::STY, ZP0}, {Instructions::STA, ZP0}, {Instructions::STX, ZP0}, {Instructions::SAX, ZP0}, {Instructions::DEY, IMP}, {Instructions::NOP, IMM}, {Instructions::TXA, IMP}, {Instructions::XAA, IMM}, {Instructions::STY, ABS}, {Instructions::STA, ABS}, {Instructions::STX, ABS}, {Instructions::SAX, ABS},
	{Instructions::BCC, REL}, {Instructions::STA, IZY}, {Instructions::KIL, IMM}, {Instructions::AHX, IZY}, {Instructions::STY, ZPX}, {Instructions::STA, ZPX}, {Instructions::STX, ZPY}, {Instructions::SAX, ZPY}, {Instructions::TYA, IMP}, {Instructions::STA, ABY}, {Instructions::TXS, IMP}, {Instructions::TAS, ABY}, {Instructions::SHY, ABX}, {Instructions::STA, ABX}, {Instructions::SHX, ABY}, {Instructions::AHX, ABY},
	{Instructions::LDY, IMM}, {Instructions::LDA, IZX}, {Instructions::LDX, IMM}, {Instructions::LAX, IZX}, {Instructions::LDY, ZP0}, {Instructions::LDA, ZP0}, {Instructions::LDX, ZP0}, {Instructions::LAX, ZP0}, {Instructions::TAY, IMP}, {Instructions::LDA, IMM}, {Instructions::TAX, IMP}, {Instructions::LAX, IMM}, {Instructions::LDY, ABS}, {Instructions::LDA, ABS}, {Instructions::LDX, ABS}, {Instructions::LAX, ABS},
	{Instructions::BCS, REL}, {Instructions::LDA, IZY}, {Instructions::KIL, IMM}, {Instructions::LAX, IZY}, {Instructions::LDY, ZPX}, {Instructions::LDA, ZPX}, {Instructions::LDX, ZPY}, {Instructions::LAX, ZPY}, {Instructions::CLV, IMP}, {Instructions::LDA, ABY}, {Instructions::TSX, IMP}, {Instructions::LAS, ABY}, {Instructions::LDY, ABX}, {Instructions::LDA, ABX}, {Instructions::LDX, ABY}, {Instructions::LAX, ABY},
	{Instructions::CPY, IMM}, {Instructions::CMP, IZX}, {Instructions::NOP, IMM}, {Instructions::DCP, IZX}, {Instructions::CPY, ZP0}, {Instructions::CMP, ZP0}, {Instructions::DEC, ZP0}, {Instructions::DCP, ZP0}, {Instructions::INY, IMP}, {Instructions::CMP, IMM}, {Instructions::DEX, IMP}, {Instructions::AXS, IMM}, {Instructions::CPY, ABS}, {Instructions::CMP, ABS}, {Instructions::DEC, ABS}, {Instructions::DCP, ABS},
	{Instructions::BNE, REL}, {Instructions::CMP, IZY}, {Instructions::KIL, IMM}, {Instructions::DCP, IZY}, {Instructions::NOP, ZPX}, {Instructions::CMP, ZPX}, {Instructions::DEC, ZPX}, {Instructions::DCP, ZPX}, {Instructions::CLD, IMP}, {Instructions::CMP, ABY}, {Instructions::NOP, IMP}, {Instructions::DCP, ABY}, {Instructions::NOP, ABX}, {Instructions::CMP, ABX}, {Instructions::DEC, ABX}, {Instructions::DCP, ABX},
	{Instructions::CPX, IMM}, {Instructions::SBC, IZX}, {Instructions::NOP, IMM}, {Instructions::ISC, IZX}, {Instructions::CPX, ZP0}, {Instructions::SBC, ZP0}, {Instructions::INC, ZP0}, {Instructions::ISC, ZP0}, {Instructions::INX, IMP}, {Instructions::SBC, IMM}, {Instructions::NOP, IMP}, {Instructions::SBC, IMM}, {Instructions::CPX, ABS}, {Instructions::SBC, ABS}, {Instructions::INC, ABS}, {Instructions::ISC, ABS},
	{Instructions::BEQ, REL}, {Instructions::SBC, IZY}, {Instructions::KIL, IMM}, {Instructions::ISC, IZY}, {Instructions::NOP, ZPX}, {Instructions::SBC, ZPX}, {Instructions::INC, ZPX}, {Instructions::ISC, ZPX}, {Instructions::SED, IMP}, {Instructions::SBC, ABY}, {Instructions::NOP, IMP}, {Instructions::ISC, ABY}, {Instructions::NOP, ABX}, {Instructions::SBC, ABX}, {Instructions::INC, ABX}, {Instructions::ISC, ABX},
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
			strPos = sprintf_s(instrStr, "%04X:  %02X  %s ", PC, fetched, names[(int)instruction.instruction]);
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
