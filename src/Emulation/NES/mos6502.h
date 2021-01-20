#pragma once
#include "saver.h"

#include <string>
#include <fstream>

// #define printDebug 1

class Bus;

enum AddressingModes : uint8_t {
	IMP, // Implied 
	IMM, // Immediate 
	ZP0, // Zero Page
	ZPX, // Zero page indexed with X
	ZPY, // Zero page indexed with Y
	REL, // Relative 
	ABS, // Absolute 
	ABX, // Absolute indexed with X
	ABY, // Absolute indexed with Y
	IND, // Indirect
	IZX, // Indexed indirect (with X)
	IZY  // Indirect indexed (with Y)
};

enum Instructions : uint8_t {
	// implementation defined. only used to inject nmi/irq
	NMI, 
	IRQ,

	// Math
	ORA, // A |= {adr}
	AND, // A &= {adr}
	EOR, // A ^= {adr}
	ADC, // A += {adr}
	SBC, // A -= {adr}
	CMP, // A - {adr}
	CPX, // X - {adr}
	CPY, // Y - {adr}
	DEC, // {adr} -= 1
	DEX, // X--
	DEY, // Y--
	INC, // {adr} += 1
	INX, // X++
	INY, // Y++
	ASL, // {adr} = {adr} << 1
	ROL, // {adr} = {adr} << 1 | C
	LSR, // {adr} = {adr} >> 2
	ROR, // {adr} = {adr} >> 2 | C * 128

	// Move
	LDA, // A = {addr}
	LDX, // X = {addr}
	LDY, // Y = {addr}
	STA, // {addr} = A 
	STX, // {addr} = X
	STY, // {addr} = Y
	TAX, // X = A
	TXA, // A = X
	TAY, // Y = A
	TYA, // A = Y
	TSX, // X = S
	TXS, // S = X
	PLA, // A = +(S)
	PHA, // (S)- = A
	PLP, // P = +(S)
	PHP, // (S)- = P

	// Jump/Flag
	BPL, // branch N == 0
	BMI, // branch N == 1
	BVC, // branch V == 0
	BVS, // branch V == 1
	BCC, // branch C == 0
	BCS, // branch C == 1
	BNE, // branch Z == 0
	BEQ, // branch Z == 1
	BRK, // (S)- = PC,P; PC = $FFFE
	RTI, // P,PC = +(S)
	JSR, // (S)- = PC; PC = {adr}
	RTS, // PC = +(S)
	JMP, // PC = {adr}
	BIT, // N = b7; V = b6; Z = A & {adr}
	CLC, // C = 0
	SEC, // C = 1 
	CLD, // D = 0
	SED, // D = 1
	CLI, // I = 0
	SEI, // I = 1
	CLV, // V = 0
	NOP,

	// Illegal
	KIL, // halts the CPU

	SLO, // {adr} = {adr} << 1; A |= {adr}
	RLA, // {adr} = {adr} rol; A &= {adr}
	SRE, // {adr} = {adr} / 2; A ^= {adr}
	RRA, // {adr} = {adr} ror; A += {adr}
	SAX, // {adr} = A & X
	DCP, // {adr} = {adr} - 1; A - {adr}
	ISC, // {adr} = {adr} + 1; A -= {adr}
	ANC, // A &= #{imm}
	ALR, // A = (A & #{imm}) >> 1
	ARR, // A = (A & #{imm}) >> 1

	AXS, // X = A & X - #{imm}

	LAS,

	// Unstable
	AHX, // {adr} = A & X & H
	SHY, // {adr} = Y & H
	SHX, // {adr} = X & H
	TAS, // S = A & X; {adr} = S & H

	// Highly unstable
	XAA, // A = X & #{imm}
	LAX, // A = X = #{imm}
};

static const char* InstructionNames[77] = {
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

struct Instruction {
	Instructions instruction;
	AddressingModes addrMode;
};

enum class State: uint8_t {
	FetchOpcode = 0,
	FetchOperator,
	WeirdRead,
	ReadAddrLo,
	ReadAddrHi,
	ReadPC,
	PageError,

	ReadInstr,
	DummyWrite,
	WriteInstr,

	StackShit1,
	StackShit2,
	StackShit3,
	StackShit4,
	StackShit5
};

class mos6502 {
	friend class CpuStateWindow;
public:
	bool IRQ;
private:
	bool NMI;

	uint8_t A;
	uint8_t X;
	uint8_t Y;
	uint8_t SP;
	uint16_t PC;

	union {
		struct {
			bool C : 1; // Carry
			bool Z : 1; // Zero
			bool I : 1; // Disable Interrupts
			bool D : 1; // Decimal Mode (unused in this implementation)
			bool B : 1; // Break
			bool U : 1; // unused
			bool V : 1; // Overflow
			bool N : 1; // Negative
		};

		uint8_t reg;
	} Status;

	State state;

	uint16_t ptr;
	uint16_t addr_abs;

	Instruction instruction;

	Bus* bus;

#ifdef printDebug
	std::ofstream file;
#endif
public:
	mos6502(Bus* bus);
	~mos6502();

	void HardReset();
	void Reset();
	void Clock();

	void Nmi();

	void SaveState(saver& saver) const;
	void LoadState(saver& saver);
private:
	void PushStack(uint8_t val);
	uint8_t PopStack();

	void ADC(uint8_t val);
	void SBC(uint8_t val);
	
	uint8_t ROL(uint8_t val);
	uint8_t ROR(uint8_t val);
};
