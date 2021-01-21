#pragma once
#include <string>

#include "../RP2A03.h"

namespace Nes {

class ApuWindow {
	friend class Core;

  private:
	bool open = false;

	std::string Title;
	RP2A03* apu;

  public:
	ApuWindow(std::string title);

	void Open() { open = true; };
	void Close() { open = false; };

	void DrawWindow();

  private:
	void DrawPulse(const Pulse& pulse, const char* label) const;
	void DrawTriangle() const;
	void DrawNoise() const;
	void DrawDMC() const;

	void DrawVrc6Pulse(const vrc6Pulse& pulse, const char* label) const;
	void DrawVrc6Saw() const;
};

}
