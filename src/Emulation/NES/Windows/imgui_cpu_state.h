#pragma once
#include <string>
#include "../mos6502.h"

class CpuStateWindow {
private:
	bool open = false;
public:
	std::string Title;
	mos6502* cpu;
public:
	CpuStateWindow(std::string title);

	void Open();
	void DrawWindow();
};

