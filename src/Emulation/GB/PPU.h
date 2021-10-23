#pragma once
#include <array>
#include <cstdint>

#include "../../RenderImage.h"
#include "../../saver.h"

namespace Gameboy {

class Gameboy;

struct Pixel {
	uint8_t id;
	uint8_t palette;
	uint8_t priority;
	bool bgPpriority;
};

class PPU {
	friend class Gameboy;
	friend class Core;
	friend class ppuWindow;

  private:
	uint8_t VRAM[2][0x2000];
	uint8_t OAM[0xA0];

	std::array<Pixel, 160> drawBuffer;
	std::array<uint8_t, 64> gbcBGP, gbcOBP;

	uint16_t windowCounter;
	int16_t LX;
	uint8_t LY;

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
	uint8_t LYC;
	uint8_t BGP;
	uint8_t OBP0, OBP1;
	uint8_t WY, WX;

	uint8_t OPRI;

	Gameboy& bus;
	RenderImage& texture;

  public:
	bool frameComplete = false;

	PPU(Gameboy& bus, RenderImage& texture) : bus(bus), texture(texture) {}

	void Reset();

	void Clock();

	void SaveState(saver& saver);
	void LoadState(saver& saver);

  private:
	void DrawBg(bool gbc);
	void DrawWindow(bool gbc);
	void DrawSprites(bool gbc);
};

}
