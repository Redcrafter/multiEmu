#pragma once
#include <string>
#include <GL/glew.h>

#include "../Emulation/ppu2C02.h"
#include "../RenderImage.h"

class PatternTables {
public:
	std::string Title;
	ppu2C02* ppu = nullptr;

	int pallet1 = 0, pallet2 = 0;
private:
	bool open = false;
	GLuint textureID;

	bool changed = false;
	Color imgData[256 * 128]{};
public:
	PatternTables(std::string title);

	void Init();
	void Open() { open = true; }
	void DrawWindow();
private:
	void DrawPatternTable();
};
