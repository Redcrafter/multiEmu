#pragma once
#include <string>

#include "../Emulation/RP2A03.h"

class ApuWindow {
private:
	bool open = false;
public:
	std::string Title;
	RP2A03* apu;
public:
	ApuWindow(std::string title);

	void Open() { open = true; };
	void Close() { open = false; };

	void DrawWindow(int available);
};
