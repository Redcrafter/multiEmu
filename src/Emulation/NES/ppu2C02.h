#pragma once
#include <cstdint>

#include <nlohmann/json.hpp>

#include "../../texture.h"
#include "Cartridge.h"

namespace Nes {

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

class ppu2C02 {
	friend class Bus;
	friend class PatternTables;
	friend class Core;

  private:
	bool oddFrame = false;
	int last2002Read = 0;

	uint8_t writeState = 0;
	uint8_t readBuffer = 0;
	// Color palettes
	std::array<uint8_t, 32> palettes;

	std::array<Sprite, 64> oam;
	std::array<Sprite, 8> oam2;
	uint8_t spriteCount;
	std::array<uint8_t, 8> spriteShifterLo, spriteShifterHi;
	bool spriteZeroPossible, spriteZeroBeingRendered;

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

	uint8_t fineX = 0;

	uint8_t bgNextTileId, bgNextTileAttrib;
	uint16_t bgNextTile;
	uint32_t bgShifterPattern, bgShifterAttrib;

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

	uint8_t ioBus = 0;
	// time till io Bus values go stale
	uint32_t reset1 = 0;
	uint32_t reset2 = 0;
	uint32_t reset3 = 0;

	int nmi = 0;
	uint8_t oamAddr = 0;

  public:
	bool frameComplete = false;

	std::shared_ptr<Mapper> cartridge;

  public:
	Texture* texture = nullptr;

	void Reset();
	void HardReset();
	void Clock();

	void SaveState(nlohmann::json& saver) const;
	void LoadState(const nlohmann::json& saver);

	// main bus
	uint8_t cpuRead(uint16_t addr, bool readOnly);
	void cpuWrite(uint16_t addr, uint8_t data);

	// ppu bus
	uint8_t ppuRead(uint16_t addr, bool readOnly = false);
	void ppuWrite(uint16_t addr, uint8_t data);

  private:
	uint8_t getPallet(uint16_t addr) const;

	Color GetPaletteColor(uint8_t palette, uint8_t pixel) const;
	void LoadBackgroundShifters();
};

}
