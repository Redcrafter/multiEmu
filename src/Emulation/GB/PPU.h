#pragma once
#include <cstdint>

#include "../../RenderImage.h"

namespace Gameboy {

class Gameboy;

class PPU {
	friend class Gameboy;
	friend class GameboyColorCore;

  private:
	int vramBank = 0;
	uint8_t VRAM[2][0x2000];
	uint8_t OAM[0xA0];

	union {
		struct {
			uint8_t modeFlag : 2;
			bool lycFlag : 1;

			bool hBlankInterrupt : 1;
			bool vBlankInterrupt : 1;
			bool oamInterrupt : 1;
			bool lycInterrupt : 1;
		};

		uint8_t reg;
	} STAT;

	union {
		struct {
			bool bgWindowEnable : 1;

			bool spriteEnable : 1;
			bool spriteSize : 1;

			bool tileMap : 1;
			bool tileData : 1;

			bool windowEnable : 1;
			bool windowTileMap : 1;

			bool lcdEnable : 1;
		};
		uint8_t reg;
	} Control;
	uint8_t SCX, SCY;
	uint16_t LX, LY;
	uint8_t LYC;
	uint8_t BGP;
	uint8_t OBP0, OBP1;
	uint8_t WY, WX;

	uint16_t windowCounter;

  public:
	bool frameComplete = false;

	Gameboy& bus;
	RenderImage& texture;

  public:
	PPU(Gameboy& gameboy, RenderImage& texture)
		: bus(gameboy), texture(texture) {}

	void Reset() {
		Control.reg = 0x91;
		STAT.reg = 1;
		SCY = 0;
		SCX = 0;
		LY = 0x91;
		LYC = 0;
		BGP = 0xFC;

		OBP0 = 0xFF;
		OBP1 = 0xFF;

		WY = 0;
		WX = 0;

		windowCounter = 0;

		LX = 120;
	}

	void Clock();

  private:
	void DrawBg();
	void DrawWindow();
	void DrawSprites();
};

}
