#pragma once
#include <cstdint>

#include "../../saver.h"

namespace Gameboy {

class Gameboy;
enum class Mode;

// fucked up shit to prevent endian issue
class Reg16 {
  private:
	uint8_t high;
	uint8_t low;

  public:
	operator uint16_t() const { return low | high << 8; }
	Reg16& operator=(uint16_t val) {
		low = val;
		high = val >> 8;

		return *this;
	}

	uint16_t operator++(int) {
		uint16_t val = *this;
		*this = val + 1;
		return val;
	}
	uint16_t operator--(int) {
		uint16_t val = *this;
		*this = val - 1;
		return val;
	}

	void operator+=(uint16_t val) {
		*this = *this + val;
	}
};
static_assert(sizeof(Reg16) == 2);

enum class CpuState {
	Normal,
	Halt,
	HaltBug,
	Stop,
};

// very similar to z80
class LR35902 {
	friend class Gameboy;
	friend class Core;

  private:
	Gameboy& bus;

	union {
		struct {
			uint8_t B;
			uint8_t C;

			uint8_t D;
			uint8_t E;

			uint8_t H;
			uint8_t L;

			uint8_t A;
			union {
				struct {
					uint8_t : 4;

					bool C : 1;
					bool H : 1;
					bool N : 1;
					bool Z : 1;
				};

				uint8_t reg;
			} F;
		};
		struct {
			Reg16 BC;
			Reg16 DE;
			Reg16 HL;
			Reg16 AF;
		};

		uint8_t reg[8];
	};

	uint16_t PC;
	uint16_t SP;

	// interrupt enable
	bool IMEtoggle;
	bool IME;
	CpuState state;

  public:
	LR35902(Gameboy& bus) : bus(bus) {}

	void Reset(Mode mode);

	void Step();

	void SaveState(saver& saver);
	void LoadState(saver& saver);

  private:
	uint8_t read(uint16_t addr);
	void write(uint16_t addr, uint8_t value);
	void cycleStall();
	void cycleOamBug(uint16_t value);

	bool checkCond(uint8_t opcode) const;

	uint8_t RightReg(uint8_t opcode);
	void WriteRightReg(uint8_t opcode, uint8_t val);

	uint16_t GetBigReg(uint8_t id) const;
	void SetBigReg(uint8_t id, uint16_t val);

	uint16_t Imm16();

	void Prefix();
};

}
