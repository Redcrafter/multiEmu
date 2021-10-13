#pragma once
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "../../NES/mos6502.h"

namespace Nes {

struct opcode {
	Instructions instruction;
	AddressingModes addrMode;

	uint32_t cycles = 0;
	uint32_t cycles_exceptions = 0;
};

struct Element {
	uint16_t address;
	// opcode insturction;

	uint8_t opcode;
	uint8_t op1, op2;

	uint16_t jumpAddr = 0;

	bool data = false;

	// std::vector<uint16_t> callers{ };
};

class DisassemblerWindow {
  private:
	std::string title;
	std::vector<Element> lines;
	bool open = false;

  public:
	DisassemblerWindow(std::string title): title(std::move(title)) {}

	void Open(const std::vector<uint8_t>& data);
	// void Load(const std::vector<uint8_t>& data);
	void DrawWindow();

  private:
	void save(const std::string& path);
};

}
