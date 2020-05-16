#pragma once
#include <string>

#include "../Emulation/ppu2C02.h"
#include "../RenderImage.h"

class PatternTables {
public:
	std::string Title;
	ppu2C02* ppu = nullptr;

	int pallet1 = 0, pallet2 = 0;
private:
	bool open = false;
	std::unique_ptr<RenderImage> image = nullptr;
public:
	PatternTables(std::string title);

	void Init();
	void Open() { open = true; }
	void DrawWindow();
private:
	void RenderSprite(int i);
	void DrawPatternTable();
};
