#pragma once
#include "../mos6502.h"
#include <string>

namespace Nes {

class CpuStateWindow {
	friend class Core;

  private:
	bool open = false;

	std::string Title;
	mos6502* cpu;

  public:
	CpuStateWindow(std::string title);

	void Open();
	void DrawWindow();
};

}
