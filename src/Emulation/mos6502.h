#pragma once
#include "../saver.h"

#include <string>
#include <fstream>

// #define printDebug

class Bus;

enum AddressingModes : uint8_t {
	IMP,
	IMM,
	ZP0,
	ZPX,
	ZPY,
	REL,
	ABS,
	ABX,
	ABY,
	IND,
	IZX,
	IZY
};

enum class Instructions : uint8_t {
	NMI,
	IRQ,

	ORA,
	AND,
	EOR,
	ADC,
	SBC,
	CMP,
	CPX,
	CPY,
	DEC,
	DEX,
	DEY,
	INC,
	INX,
	INY,
	ASL,
	ROL,
	LSR,
	ROR,

	// Move
	LDA,
	STA,
	LDX,
	STX,
	LDY,
	STY,
	TAX,
	TXA,
	TAY,
	TYA,
	TSX,
	TXS,
	PLA,
	PHA,
	PLP,
	PHP,

	// Jump/Flag
	BPL,
	BMI,
	BVC,
	BVS,
	BCC,
	BCS,
	BNE,
	BEQ,
	BRK,
	RTI,
	JSR,
	RTS,
	JMP,
	BIT,
	CLC,
	SEC,
	CLD,
	SED,
	CLI,
	SEI,
	CLV,
	NOP,

	// Illegal
	KIL,

	SLO,
	RLA,
	SRE,
	RRA,
	SAX,
	DCP,
	ISC,
	ANC,
	ALR,
	ARR,
	XAA,
	LAX, // Highly unstable
	AXS,
	AHX, // Unstable
	SHY, // Unstable
	SHX, // Unstable
	TAS, // Unstable
	LAS
};

struct Instruction {
	Instructions instruction;
	AddressingModes addrMode;
};

enum class State {
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

struct mos6502state {
	bool NMI = false;

	uint8_t A = 0;
	uint8_t X = 0;
	uint8_t Y = 0;
	uint8_t SP = 0xFD;
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

		uint8_t reg = 0b00100100;
	} Status;

	State state = State::FetchOpcode;

	uint16_t ptr;
	uint16_t addr_abs;

	Instruction instruction;

	uint64_t cycles = 0;
};

class mos6502 : mos6502state {
	friend class CpuStateWindow;
public:
	bool IRQ = false;
private:
	Bus* bus;
public:
	mos6502(Bus* bus);
	~mos6502();

	void Clock();
	void Reset();

	void Nmi();

	void SaveState(saver& saver) const;
	void LoadState(saver& saver);
private:
	void PushStack(uint8_t val);
	uint8_t PopStack();

#ifdef printDebug
	std::ofstream file;
#endif

	void ORA(uint8_t val);
	void AND(uint8_t val);
	void EOR(uint8_t val);
	void ADC(uint8_t val);
	void SBC(uint8_t val);
	
	uint8_t ASL(uint8_t val);
	uint8_t ROL(uint8_t val);
	uint8_t LSR(uint8_t val);
	uint8_t ROR(uint8_t val);

	void CMP(uint8_t val);
	void CPX(uint8_t val);
	void CPY(uint8_t val);

	void LDA(uint8_t val);
	void LDX(uint8_t val);
	void LDY(uint8_t val);

	void LAX(uint8_t val);
};
