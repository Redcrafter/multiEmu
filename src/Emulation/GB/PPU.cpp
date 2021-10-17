#include "PPU.h"

#include "../../math.h"
#include "Gameboy.h"

namespace Gameboy {

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

void PPU::Clock(Gameboy& bus, RenderImage& texture) {
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

		// mode 2: 0-79
		if(LY < 144) {
			STAT.modeFlag = 2;
			if(STAT.oamInterrupt) bus.Interrupt(Interrupt::LCDStat);
		}
	}

	if(LY < 144) {
		// mode 3: 80-251
		if(LX == 80) {
			STAT.modeFlag = 3;

			if(Control.lcdEnable) {
				if(bus.gbc) {
					DrawBg(true);
					DrawWindow(true);
					DrawSprites(true);

					for(size_t i = 0; i < 160; i++) {
						auto el = drawBuffer[i];
						auto id = ((el.palette & 7) * 4 + el.id) * 2;

						Color c;
						uint16_t col;
						if(el.palette & 0x80) { // sprite
							col = gbcOBP[id] | gbcOBP[id + 1] << 8;
						} else { // bg/window
							col = gbcBGP[id] | gbcBGP[id + 1] << 8;
						}

						Color color { (col << 3) & 0xF8, (col >> 2) & 0xF8, (col >> 7) & 0xF8 };
						texture.SetPixel(i, LY, color);
					}
				} else {
					if(Control.bgWindowEnable) {
						DrawBg(false);
						DrawWindow(false);
					} else {
						std::fill(drawBuffer.begin(), drawBuffer.end(), Uhhh { 0, 0 });
					}
					DrawSprites(false);

					for(size_t i = 0; i < 160; i++) {
						auto el = drawBuffer[i];
						auto color = (el.palette >> (el.id << 1)) & 3;
						texture.SetPixel(i, LY, palette[color]);
					}
				}
			} else {
				for(size_t i = 0; i < 160; i++) {
					texture.SetPixel(i, LY, { 0xFF, 0xFF, 0xFF });
				}
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

void PPU::DrawBg(bool gbc) {
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

	auto prioOverwrite = gbc && !Control.bgWindowEnable;

	for(int i = 0; i < 160;) {
		auto tile = VRAM[0][mapOffs + lineoffs];
		auto attr = VRAM[1][mapOffs + lineoffs];

		uint8_t pal = gbc ? attr & 7 : BGP;
		auto vramBank = (attr & 8) != 0;
		bool prio = attr & 0x80 && gbc && Control.bgWindowEnable;

		uint16_t addr = (Control.tileData ? tile : 0x100 + int8_t(tile)) << 4;
		if(attr & 0x40) { // vertical flip
			addr |= (7 - y) << 1;
		} else {
			addr |= y << 1;
		}
		auto low = VRAM[vramBank][addr];
		auto high = VRAM[vramBank][addr + 1];
		if(attr & 0x20) { // horizontal flip
			low = math::reverse(low);
			high = math::reverse(high);
		}
		uint16_t merged = math::interleave(low, high) << (2 * x);

		while(x < 8 && i < 160) {
			drawBuffer[i] = { (uint8_t)(merged >> 14), pal, 0, prio };
			merged <<= 2;
			x++;
			i++;
		}
		x = 0;
		lineoffs = (lineoffs + 1) & 31;
	}
}

void PPU::DrawWindow(bool gbc) {
	if(!Control.windowEnable) return;

	auto x = WX - 7;
	auto y = (LY - WY) & 7;

	if(WY <= LY && x < 160) {
		auto mapOffs = Control.windowTileMap ? 0x1C00 : 0x1800;
		mapOffs += (windowCounter & 0xF8) << 2;
		windowCounter++;

		int lineoffs = 0;
		for(size_t i = x; i < 160;) {
			auto tile = VRAM[0][mapOffs + lineoffs];
			auto attr = VRAM[1][mapOffs + lineoffs];

			uint8_t pal = gbc ? attr & 7 : BGP;
			auto vramBank = (attr & 8) != 0;
			bool prio = attr & 0x80 && gbc && Control.bgWindowEnable;

			uint16_t addr = (Control.tileData ? tile : 0x100 + int8_t(tile)) << 4;
			if(attr & 0x40) { // vertical flip
				addr |= (7 - y) << 1;
			} else {
				addr |= y << 1;
			}
			auto low = VRAM[vramBank][addr];
			auto high = VRAM[vramBank][addr + 1];
			if(attr & 0x20) { // horizontal flip
				low = math::reverse(low);
				high = math::reverse(high);
			}
			uint16_t merged = math::interleave(low, high);

			for(size_t x = 0; x < 8 && i < 160; x++) {
				drawBuffer[i] = { (uint8_t)(merged >> 14), pal, 0, prio };
				merged <<= 2;
				i++;
			}

			lineoffs++;
		}
	}
}

void PPU::DrawSprites(bool gbc) {
	if(!Control.spriteEnable) return;

	auto spriteSize = Control.spriteSize ? 16 : 8;

	int spriteCount = 0;
	for(int i = 0; i < 40 && spriteCount < 10; i++) {
		auto s = ((Sprite*)OAM)[i];

		// relative y position
		auto y = LY - (s.Y - 16);
		auto x = s.X - 8;

		if(y < 0 || y >= spriteSize) continue;

		spriteCount++;
		if(s.X == 0 || s.X >= 160) continue;

		// used pallet
		uint8_t prio;
		uint8_t pal;
		int tileBank;
		if(gbc) {
			pal = s.cgbPalletNumber | 0x80;
			tileBank = s.vramBank;
			prio = 40 - i + 1;
		} else {
			pal = s.gbPalletNumber ? OBP1 : OBP0;
			tileBank = 0;
			prio = 160 - s.X + 1;
		}

		// address of used tile. aligned to even space when 8x16
		int addr = (s.Number & ~(int)Control.spriteSize) << 4;
		if(s.verticalFlip) {
			addr |= (spriteSize - 1 - y) << 1;
		} else {
			addr |= y << 1;
		}

		uint8_t low = VRAM[tileBank][addr];
		uint8_t high = VRAM[tileBank][addr | 1];

		if(s.horizontalFlip) {
			low = math::reverse(low);
			high = math::reverse(high);
		}
		// interlave low and high
		auto merged = math::interleave(low, high);
		auto e = std::min((int)s.X, 160);

		if(x < 0) {
			merged <<= 2 * -x;
			x = 0;
		}

		while(x < e) {
			uint8_t id = merged >> 14;

			auto bg = drawBuffer[x];
			if(id != 0 && (bg.id == 0 || (!bg.bgPpriority && !s.priority && prio > bg.priority))) {
				drawBuffer[x] = { id, pal, prio };
			}

			merged <<= 2;
			x++;
		}
	}
}

}
