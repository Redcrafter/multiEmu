#pragma once
#include <string>

#include "../../../texture.h"
#include "../ppu2C02.h"

namespace Nes {

class PatternTables {
	friend class Core;

  private:
	std::string Title;
	ppu2C02* ppu = nullptr;

	int pallet1 = 0, pallet2 = 0;

	bool open = false;
	Texture image;

  public:
	PatternTables(std::string title);

	void Open() { open = true; }
	void DrawWindow();

  private:
	void RenderSprite(int i);
	void DrawPatternTable();
};

}
