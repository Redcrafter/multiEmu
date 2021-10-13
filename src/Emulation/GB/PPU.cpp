#include "PPU.h"

#include "Gameboy.h"

namespace Gameboy {

static uint8_t reverse(uint8_t b) {
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	return b;
}

static uint16_t interleave(uint8_t low, uint8_t high) {
	uint16_t x = low;
	uint16_t y = high;

	// x = (x | (x << 8)) & 0x00FF00FF;
	x = (x | (x << 4)) & 0x0F0F0F0F;
	x = (x | (x << 2)) & 0x33333333;
	x = (x | (x << 1)) & 0x55555555;

	// y = (y | (y << 8)) & 0x00FF00FF;
	y = (y | (y << 4)) & 0x0F0F0F0F;
	y = (y | (y << 2)) & 0x33333333;
	y = (y | (y << 1)) & 0x55555555;

	return x | (y << 1);
}

#define getTileAddr() Control.tileData ? (tile << 4) | (y << 1) : 0x1000 + int8_t(tile) * 16 + (y << 1)

/*const Color palette[] = {
	{ 0x00, 0x3f, 0x00 },
	{ 0x2E, 0x73, 0x20 },
	{ 0x8C, 0xbf, 0x0A },
	{ 0xA0, 0xCF, 0x0A }
};*/
const Color palette[] = {
	{ 0xFF, 0xFF, 0xFF },
	{ 0xAA, 0xAA, 0xAA },
	{ 0x55, 0x55, 0x55 },
	{ 0x00, 0x00, 0x00 }
};

struct Sprite {
	uint8_t Y;
	uint8_t X;
	uint8_t Number;

	struct {
		uint8_t cgbPalletNumber : 3;
		bool vramBank : 1;
		bool gbPalletNumber : 1;
		bool horizontalFlip : 1;
		bool verticalFlip : 1;
		bool priority : 1;
	};
};

void PPU::Clock() {
	LX += 4;
	if(LX == 456) {
		LX = 0;
		LY = (LY + 1) % 154;

		if(LY == 144) {
			STAT.modeFlag = 1;
			windowCounter = 0;
			bus.Interrupt(Interrupt::VBlank);
			if(STAT.vBlankInterrupt) bus.Interrupt(Interrupt::LCDStat);

			frameComplete = true;
		}
	}

	if(LY < 144) {
		// mode 2: 0-79
		if(LX == 0) {
			STAT.modeFlag = 2;
			if(STAT.oamInterrupt) bus.Interrupt(Interrupt::LCDStat);
		}

		// mode 3: 80-251
		if(LX == 80) {
			STAT.modeFlag = 3;

			if(Control.lcdEnable) {
				DrawBg();
				DrawWindow();
				DrawSprites();
			}
		}

		// mode 0: 252-456
		if(LX == 252) {
			STAT.modeFlag = 0;
			if(STAT.hBlankInterrupt) bus.Interrupt(Interrupt::LCDStat);
		}
	}

	if(LY == LYC && !STAT.lycFlag && STAT.lycInterrupt) {
		bus.Interrupt(Interrupt::LCDStat);
	}
	STAT.lycFlag = LY == LYC;
}

void PPU::DrawBg() {
	if(!Control.bgWindowEnable) {
		for(int i = 0; i < 160; ++i) {
			texture.SetPixel(i, LY, Color { 255, 255, 255 });
		}
		return;
	}

	// VRAM offset for the tile map
	auto mapOffs = Control.tileMap ? 0x1C00 : 0x1800;

	// Which line of tiles to use in the map
	// mapOffs += (((LY + SCY) & 255) >> 3) << 5;
	mapOffs += ((LY + SCY) & 0xF8) << 2;

	// Which tile to start with in the map line
	auto lineoffs = SCX >> 3;
	// Which line of pixels to use in the tiles
	auto y = (LY + SCY) & 7;
	// Where in the tileline to start
	auto x = SCX & 7;

	for(int i = 0; i < 160;) {
		auto tile = VRAM[0][mapOffs + lineoffs];

		uint16_t addr = getTileAddr();
		auto merged = interleave(VRAM[0][addr], VRAM[0][addr + 1]);

		while(x < 8) {
			auto color = (merged >> ((7 - x) * 2)) & 3;
			color = (BGP >> (color << 1)) & 3;
			texture.SetPixel(i, LY, palette[color]);

			x++;
			i++;
		}
		x = 0;
		lineoffs = (lineoffs + 1) & 31;
	}
}

void PPU::DrawWindow() {
	if(!Control.windowEnable || !Control.bgWindowEnable) return;

	auto x = WX - 7;
	auto y = (LY - WY) & 7;

	if(WY <= LY && x < 160) {
		auto mapOffs = Control.windowTileMap ? 0x1C00 : 0x1800;
		mapOffs += (windowCounter & 0xF8) << 2;
		windowCounter++;

		int lineoffs = 0;
		for(size_t i = x; i < 160;) {
			auto tile = VRAM[0][mapOffs + lineoffs];

			uint16_t addr = getTileAddr();
			auto merged = interleave(VRAM[0][addr], VRAM[0][addr + 1]);

			for(size_t j = 0; j < 8; j++) {
				auto id = merged >> 14;
				auto color = (BGP >> (id << 1)) & 3;
				merged <<= 2;
				texture.SetPixel(i, LY, palette[color]);

				i++;
			}

			lineoffs++;
		}
	}
}

void PPU::DrawSprites() {
	if(!Control.spriteEnable) return;

	auto spriteSize = Control.spriteSize ? 16 : 8;

	int spriteCount = 0;
	std::array<Sprite, 10> spriteBuffer;
	for(size_t i = 0; i < 40 && spriteCount < 10; i++) {
		auto s = ((Sprite*)OAM)[i];

		// relative y position
		auto y = LY - (s.Y - 16);

		// is sprite on current line?
		if(y < 0 || y >= spriteSize) continue;

		// stable insertion sort
		int insertPos = spriteCount;
		while(insertPos > 0) {
			if(spriteBuffer[insertPos - 1].X > s.X) break;

			spriteBuffer[insertPos] = spriteBuffer[insertPos - 1];
			insertPos--;
		}
		spriteBuffer[insertPos] = s;

		spriteCount++;
	}

	for(int i = 0; i < spriteCount; i++) {
		auto s = spriteBuffer[i];
		// relative y position
		auto y = LY - (s.Y - 16);

		// used pallet
		auto pal = s.gbPalletNumber ? OBP1 : OBP0;

		// address of used tile. aligned to even space when 8x16
		int addr = (s.Number & ~(int)Control.spriteSize) << 4;
		if(s.verticalFlip) {
			addr |= (spriteSize - 1 - y) << 1;
		} else {
			addr |= y << 1;
		}

		uint8_t low = VRAM[0][addr];
		uint8_t high = VRAM[0][addr | 1];

		// reversed by default so we can use & 3
		if(s.horizontalFlip) {
			low = reverse(low);
			high = reverse(high);
		}
		// interlave low and high
		auto merged = interleave(low, high);

		for(int j = -8; j < 0; j++) {
			auto id = merged >> 14;
			auto val = (pal >> (id << 1)) & 3;
			merged <<= 2;
			if(val != 0) texture.SetPixel(j + s.X, LY, palette[val]);
		}
	}
}

}
