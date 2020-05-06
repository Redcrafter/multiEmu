#pragma once
#include <cstdint>
#include "Cartridge.h"
#include "../RenderImage.h"
#include "../saver.h"

class PatternTables;
class Bus;

struct Sprite {
	uint8_t y = 0;
	uint8_t id = 0;

	union {
		struct {
			uint8_t Palette : 2;
			uint8_t Unused : 3;
			bool Priority : 1;
			bool FlipHorizontal : 1;
			bool FlipVertical : 1;
		};

		uint8_t reg = 0;
	} Attributes;

	uint8_t x = 0;
};

struct PpuState {
	bool oddFrame = false;
	int last2002Read = 0;

	uint8_t writeState = 0;
	uint8_t readBuffer = 0;
	// Color palettes
	uint8_t palettes[32]{};
	// Nametables
	uint8_t vram[0x1000]{};
	// For cartridges without ChrRom
	uint8_t chrRAM[8 * 1024]{};

	Sprite oam[64]{}, oam2[8]{};
	uint8_t spriteCount = 0;
	uint8_t spriteShifterLo[8]{}, spriteShifterHi[8]{};
	bool spriteZeroPossible = false, spriteZeroBeingRendered = false;

	int scanlineX = 0, scanlineY = 241;

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

		uint8_t reg = 0;
	} Mask;

	union {
		struct {
			uint8_t unused : 5;
			uint8_t spriteOverflow : 1;
			uint8_t sprite0Hit : 1;
			uint8_t VerticalBlank : 1;
		};

		uint8_t reg = 0xA0;
	} Status;

	uint8_t fineX = 0;

	uint8_t bgNextTileId = 0, bgNextTileAttrib = 0, bgNextTileLsb = 0, bgNextTileMsb = 0;
	uint16_t bgShifterPatternLo = 0, bgShifterPatternHi = 0, bgShifterAttribLo = 0, bgShifterAttribHi = 0;

	union {
		struct {
			uint16_t coarseX : 5;
			uint16_t coarseY : 5;
			uint16_t nametableX : 1;
			uint16_t nametableY : 1;
			uint16_t fineY : 3;
			uint16_t unused : 1;
		};

		uint16_t reg = 0;
	} vramAddr, tramAddr;
};

class ppu2C02 : PpuState {
	friend class MemoryEditor;
public:
	bool frameComplete = false;
	int nmi = 0;
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

		uint8_t reg = 0;
	} Control;

public:
	RenderImage* texture;
	std::shared_ptr<Mapper> cartridge;
	uint8_t* pOAM = reinterpret_cast<uint8_t*>(oam);

	ppu2C02();
	void Reset();
	void Clock();

	void SaveState(saver& saver);
	void LoadState(saver& saver);

	// main bus
	uint8_t cpuRead(uint16_t addr, bool readOnly);
	void cpuWrite(uint16_t addr, uint8_t data);

	// ppu bus
	uint8_t ppuRead(uint16_t addr, bool readOnly = false);
	void ppuWrite(uint16_t addr, uint8_t data);

	// void DrawPatternTable(PatternTables& texture, int i, int palette);
	Color GetPaletteColor(uint8_t palette, uint8_t pixel) const;
private:
	void LoadBackgroundShifters();
	uint8_t& getRef(uint16_t addr);	
};
