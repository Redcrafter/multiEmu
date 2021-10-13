#pragma once
#include <array>
#include <string>
#include <vector>

#include "chip8.h"

namespace Chip8 {

struct Element {
	uint16_t address;
	uint16_t opcode;

	bool data;
};

class DisassemblerWindow {
  private:
	Chip8& chip8;
	std::vector<Element> elements;

	bool open = false;

  public:
	DisassemblerWindow(Chip8& chip8) : chip8(chip8) {}

	void Open();
	void Update();
	void DrawWindow();

  private:
	void save(const std::string& path);
};

}
