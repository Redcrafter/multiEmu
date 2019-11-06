#pragma once
#include <cstdint>
#include "Cartridge.h"
#include "../RenderImage.h"
#include "../saver.h"

class Bus;

struct Sprite {
	uint8_t y;
	uint8_t id;
	union {
		struct {
			uint8_t Palette : 2;
			uint8_t Unused : 3;
			bool Priority : 1;
			bool FlipHorizontal : 1;
			bool FlipVertical : 1;
		};
		uint8_t reg;
	} Attributes;
	uint8_t x;
};

struct PpuState {
	bool oddFrame = false;
	int last2002Read = 0;

	uint8_t writeState;
	uint8_t readBuffer;
	uint8_t palettes[32], vram[2 * 1024], chrRAM[8 * 1024];

	Sprite oam[64], oam2[8];
	uint8_t spriteCount;
	uint8_t spriteShifterLo[8], spriteShifterHi[8];
	bool spriteZeroPossible, spriteZeroBeingRendered;

	int scanlineX, scanlineY;
	
	union {
		struct {
			bool grayscale : 1;
			bool backgroundLeft : 1;
			bool spriteLeft : 1;
			bool renderBackground : 1;
			bool renderSprites : 1;
			bool emphasizeRed : 1;
			bool emphasizeGreen : 1;
			bool emphasizeBlue : 1;
		};

		uint8_t reg;
	} Mask;

	union {
		struct {
			uint8_t unused : 5;
			uint8_t spriteOverflow : 1;
			uint8_t sprite0Hit : 1;
			uint8_t VerticalBlank : 1;
		};

		uint8_t reg;
	} Status;

	uint8_t fineX;

	uint8_t bgNextTileId, bgNextTileAttrib, bgNextTileLsb, bgNextTileMsb;
	uint16_t bgShifterPatternLo, bgShifterPatternHi, bgShifterAttribLo, bgShifterAttribHi;

	union {
		struct {
			uint16_t coarseX : 5;
			uint16_t coarseY : 5;
			uint16_t nametableX : 1;
			uint16_t nametableY : 1;
			uint16_t fineY : 3;
			uint16_t unused : 1;
		};

		uint16_t reg;
	} vramAddr, tramAddr;
};

class ppu2C02 : PpuState {
public:
	bool frameComplete = false;
	int nmi;
	uint8_t oamAddr = 0;
	
	union {
		struct {
			uint8_t nametableX : 1;
			uint8_t nametableY : 1;
			uint8_t vramIncrement : 1;
			uint8_t patternSprite : 1;
			uint8_t patternBackground : 1;
			uint8_t spriteSize : 1;
			uint8_t slaveMode : 1;
			uint8_t enableNMI : 1;
		};

		uint8_t reg;
	} Control;
public:
	RenderImage* texture;
	std::shared_ptr<Cartridge> cartridge;
	uint8_t* pOAM = reinterpret_cast<uint8_t*>(oam);

	ppu2C02();
	void Reset();
	void Clock();

	void SaveState(saver& saver);
	void LoadState(saver& saver);

	void DrawPatternTable(RenderImage* texture, int i, int palette);

	// main bus
	uint8_t cpuRead(uint16_t addr);
	void cpuWrite(uint16_t addr, uint8_t data);

	// ppu bus
	uint8_t ppuRead(uint16_t addr);
	void ppuWrite(uint16_t addr, uint8_t data);
private:
	void LoadBackgroundShifters();
	Color GetPaletteColor(uint8_t palette, uint8_t pixel) const;
};
