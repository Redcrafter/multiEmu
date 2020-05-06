#include "ppu2C02.h"
#include "Bus.h"

#include <iostream>
#include <cassert>
#include <cstring>

const Color colors[] = {
	{84, 84, 84},
	{0, 30, 116},
	{8, 16, 144},
	{48, 0, 136},
	{68, 0, 100},
	{92, 0, 48},
	{84, 4, 0},
	{60, 24, 0},
	{32, 42, 0},
	{8, 58, 0},
	{0, 64, 0},
	{0, 60, 0},
	{0, 50, 60},
	{0, 0, 0},
	{0, 0, 0},
	{0, 0, 0},
	{152, 150, 152},
	{8, 76, 196},
	{48, 50, 236},
	{92, 30, 228},
	{136, 20, 176},
	{160, 20, 100},
	{152, 34, 32},
	{120, 60, 0},
	{84, 90, 0},
	{40, 114, 0},
	{8, 124, 0},
	{0, 118, 40},
	{0, 102, 120},
	{0, 0, 0},
	{0, 0, 0},
	{0, 0, 0},
	{236, 238, 236},
	{76, 154, 236},
	{120, 124, 236},
	{176, 98, 236},
	{228, 84, 236},
	{236, 88, 180},
	{236, 106, 100},
	{212, 136, 32},
	{160, 170, 0},
	{116, 196, 0},
	{76, 208, 32},
	{56, 204, 108},
	{56, 180, 204},
	{60, 60, 60},
	{0, 0, 0},
	{0, 0, 0},
	{236, 238, 236},
	{168, 204, 236},
	{188, 188, 236},
	{212, 178, 236},
	{236, 174, 236},
	{236, 174, 212},
	{236, 180, 176},
	{228, 196, 144},
	{204, 210, 120},
	{180, 222, 120},
	{168, 226, 144},
	{152, 226, 180},
	{160, 214, 228},
	{160, 162, 160},
	{0, 0, 0},
	{0, 0, 0}
};

static uint8_t FlipByte(uint8_t b) {
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	return b;
}

ppu2C02::ppu2C02() { }

void ppu2C02::Reset() {
	Control.reg = 0;
	Mask.reg = 0;
	// Status.reg = 0xA0;

	writeState = 0;
	fineX = 0;

	readBuffer = 0;
	last2002Read = 0;

	oddFrame = false;

	scanlineX = 0;
	scanlineY = 241;
}

void ppu2C02::Clock() {
	last2002Read++;

	if(scanlineY == 0 && scanlineX == 0 && oddFrame && Mask.renderBackground) {
		// "Odd Frame" cycle skip
		scanlineX = 1;
	}

	if(scanlineY == -1 && scanlineX == 1) {
		Status.reg = 0;

		for(int i = 0; i < 8; ++i) {
			spriteShifterLo[i] = 0;
			spriteShifterHi[i] = 0;
		}
	}

	if(scanlineY < 240) {
		if((scanlineX >= 1 && scanlineX <= 256) || (scanlineX >= 321 && scanlineX <= 336)) {
			if(Mask.renderBackground) {
				bgShifterPatternLo <<= 1;
				bgShifterPatternHi <<= 1;

				bgShifterAttribLo <<= 1;
				bgShifterAttribHi <<= 1;
			}

			if(scanlineX >= 2 && scanlineX <= 256 && Mask.renderSprites) {
				for(int i = 0; i < spriteCount; i++) {
					if(oam2[i].x > 0) {
						oam2[i].x--;
					} else {
						spriteShifterLo[i] <<= 1;
						spriteShifterHi[i] <<= 1;
					}
				}
			}

			switch((scanlineX - 1) % 8) {
				case 0:
					LoadBackgroundShifters();
					bgNextTileId = ppuRead(0x2000 | (vramAddr.reg & 0x0FFF));
					break;
				case 2:
					bgNextTileAttrib = ppuRead(0x23C0 |
						(vramAddr.nametableY << 11) |
						(vramAddr.nametableX << 10) |
						((vramAddr.coarseY >> 2) << 3) |
						(vramAddr.coarseX >> 2)
					);

					if(vramAddr.coarseY & 2)
						bgNextTileAttrib >>= 4;
					if(vramAddr.coarseX & 2)
						bgNextTileAttrib >>= 2;
					bgNextTileAttrib &= 0x03;
					break;
				case 4:
					bgNextTileLsb = ppuRead((Control.patternBackground << 12)
						+ (bgNextTileId << 4)
						+ vramAddr.fineY);
					break;
				case 6:
					bgNextTileMsb = ppuRead((Control.patternBackground << 12)
						+ (bgNextTileId << 4)
						+ vramAddr.fineY + 8);
					break;
				case 7:
					if(Mask.renderBackground || Mask.renderSprites) {
						if(vramAddr.coarseX == 31) {
							vramAddr.coarseX = 0;
							vramAddr.nametableX = ~vramAddr.nametableX;
						} else {
							vramAddr.coarseX++;
						}
					}
					break;
			}
		}

		switch(scanlineX) {
			case 256:
				if(Mask.renderBackground || Mask.renderSprites) {
					if(vramAddr.fineY < 7) {
						vramAddr.fineY++;
					} else {
						vramAddr.fineY = 0;
						if(vramAddr.coarseY == 29) {
							vramAddr.coarseY = 0;
							vramAddr.nametableY = ~vramAddr.nametableY;
						} else if(vramAddr.coarseY == 31) {
							vramAddr.coarseY = 0;
						} else {
							vramAddr.coarseY++;
						}
					}
				}
				break;
			case 257:
				LoadBackgroundShifters();

				if(Mask.renderBackground || Mask.renderSprites) {
					vramAddr.nametableX = tramAddr.nametableX;
					vramAddr.coarseX = tramAddr.coarseX;
				}

				if(scanlineY >= 0) {
					memset(oam2, 0xFF, sizeof(oam2));
					spriteCount = 0;
					spriteZeroPossible = false;

					for(int i = 0; i < 64 && spriteCount < 9; i++) {
						const auto item = oam[i];
						auto diff = scanlineY - item.y;

						if(diff >= 0 && diff < (Control.spriteSize ? 16 : 8)) {
							if(spriteCount < 8) {
								if(i == 0) {
									spriteZeroPossible = true;
								}
								oam2[spriteCount] = item;
								spriteCount++;
							} else {
								if(Mask.renderSprites || Mask.renderBackground) {
									Status.spriteOverflow = true;
								}
							}
						}
					}
				}
				break;
			case 280:
			case 281:
			case 282:
			case 283:
			case 284:
			case 285:
			case 286:
			case 287:
			case 288:
			case 289:
			case 290:
			case 291:
			case 292:
			case 293:
			case 294:
			case 295:
			case 296:
			case 297:
			case 298:
			case 299:
			case 300:
			case 301:
			case 302:
			case 303:
			case 304:
				if(scanlineY == -1) {
					if(Mask.renderBackground || Mask.renderSprites) {
						vramAddr.fineY = tramAddr.fineY;
						vramAddr.nametableY = tramAddr.nametableY;
						vramAddr.coarseY = tramAddr.coarseY;
					}
				}
				break;
			case 338:
			case 340:
				bgNextTileId = ppuRead(0x2000 | (vramAddr.reg & 0x0FFF));
				break;
		}

		if(scanlineX >= 257 && scanlineX <= 320) {
			int i = (scanlineX - 257) / 8;
			const auto sprite = oam2[i];
			uint8_t bits;
			uint16_t addr;

			if(!Control.spriteSize) {
				// 8x8 
				addr = (Control.patternSprite << 12) | (sprite.id << 4);
			} else {
				// 8x16
				addr = (sprite.id & 1) << 12;

				if(scanlineY - sprite.y < 8) {
					addr |= ((sprite.id & 0xFE) << 4);
				} else {
					addr |= (((sprite.id & 0xFE) + 1) << 4);
				}
			}

			if(sprite.Attributes.FlipVertical) {
				addr |= (7 - (scanlineY - sprite.y)) & 0x7;
			} else {
				addr |= (scanlineY - sprite.y) & 0x7;
			}

			switch((scanlineX - 1) % 8) {
				case 0:
				case 2:
					// TODO: Garbage fetch?
					break;
				case 4:
					bits = ppuRead(addr);

					if(sprite.Attributes.FlipHorizontal) {
						bits = FlipByte(bits);
					}

					spriteShifterLo[i] = bits;
					break;
				case 6:
					bits = ppuRead(addr + 8);

					if(sprite.Attributes.FlipHorizontal) {
						bits = FlipByte(bits);
					}

					spriteShifterHi[i] = bits;
					break;
			}
		}
	}

	if(scanlineY == 241 && scanlineX == 1 && last2002Read != 1) {
		Status.VerticalBlank = true;
		if(Control.enableNMI) {
			nmi = 1;
		}
	}

	uint8_t bgPixel = 0;
	uint8_t bgPalette = 0;

	if(Mask.renderBackground) {
		// Handle Pixel Selection by selecting the relevant bit
		// depending upon fine x scolling. This has the effect of
		// offsetting ALL background rendering by a set number
		// of pixels, permitting smooth scrolling
		uint16_t bit_mux = 0x8000 >> fineX;

		// Select Plane pixels by extracting from the shifter
		// at the required location.
		uint8_t p0_pixel = (bgShifterPatternLo & bit_mux) > 0;
		uint8_t p1_pixel = (bgShifterPatternHi & bit_mux) > 0;

		// Combine to form pixel index
		bgPixel = (p1_pixel << 1) | p0_pixel;

		// Get palette
		uint8_t bg_pal0 = (bgShifterAttribLo & bit_mux) > 0;
		uint8_t bg_pal1 = (bgShifterAttribHi & bit_mux) > 0;
		bgPalette = (bg_pal1 << 1) | bg_pal0;
	}

	uint8_t fgPixel = 0;
	uint8_t fgPalette = 0;
	uint8_t fgPriority = 0;

	if(Mask.renderSprites) {
		spriteZeroBeingRendered = false;
		for(int i = 0; i < spriteCount; ++i) {
			const auto sprite = oam2[i];

			if(sprite.x == 0) {
				uint8_t p0_pixel = (spriteShifterLo[i] & 0x80) > 0;
				uint8_t p1_pixel = (spriteShifterHi[i] & 0x80) > 0;

				// Combine to form pixel index
				fgPixel = (p1_pixel << 1) | p0_pixel;

				fgPalette = sprite.Attributes.Palette + 4;
				fgPriority = !sprite.Attributes.Priority;

				if(fgPixel != 0) {
					if(i == 0) {
						spriteZeroBeingRendered = true;
					}
					break;
				}
			}
		}
	}

	uint8_t pixel = 0;
	uint8_t palette = 0;

	if(bgPixel == 0) {
		if(fgPixel != 0) {
			pixel = fgPixel;
			palette = fgPalette;
		}
	} else if(fgPixel == 0) {
		pixel = bgPixel;
		palette = bgPalette;
	} else {
		if(fgPriority) {
			pixel = fgPixel;
			palette = fgPalette;
		} else {
			pixel = bgPixel;
			palette = bgPalette;
		}

		if(spriteZeroPossible && spriteZeroBeingRendered &&
			Mask.renderBackground && Mask.renderSprites &&
			((Mask.backgroundLeft && Mask.spriteLeft || scanlineX >= 9) && scanlineX < 256)) {
			// ScanlineX >= 2
			Status.sprite0Hit = true;
		}
	}

	texture->SetPixel(scanlineX - 1, scanlineY, GetPaletteColor(palette, pixel));

	scanlineX++;
	if(scanlineX >= 341) {
		scanlineX = 0;
		scanlineY++;

		if(scanlineY >= 261) {
			scanlineY = -1;

			frameComplete = true;
			oddFrame = !oddFrame;
		}
	}
}

void ppu2C02::SaveState(saver& saver) {
	assert(!frameComplete);
	saver << nmi;
	saver << oamAddr;
	saver << Control.reg;

	saver.Write(reinterpret_cast<char*>(this), sizeof(PpuState));
}

void ppu2C02::LoadState(saver& saver) {
	frameComplete = false;
	saver >> nmi;
	saver >> oamAddr;
	saver >> Control.reg;

	saver.Read(reinterpret_cast<char*>(this), sizeof(PpuState));
}

uint8_t ppu2C02::cpuRead(uint16_t addr, bool readOnly) {
	uint8_t res = 0;

	switch(addr) {
		case 0x2002:
			res = (Status.reg & 0xE0) | (readBuffer & 0x1F);
			if(!readOnly) {
				last2002Read = 0;

				Status.VerticalBlank = false; // Clear vblank
				writeState = 0;               // Clear write latch
				if(nmi == 1 || nmi == 2) {
					nmi = 0; // Reading 2002 at same time as nmi is set supresses it
				}
			}
			break;
		case 0x2004: return pOAM[oamAddr];
		case 0x2007:
			res = readBuffer;
			if(!readOnly) {
				readBuffer = ppuRead(vramAddr.reg);
				
				if(vramAddr.reg >= 0x3F00) {
					res = readBuffer; // palette
					// Read buffer is mirrored into vram
					// using getRef to not trigger cartridge
					readBuffer = getRef(vramAddr.reg - 0x1000);
				}

				vramAddr.reg += Control.vramIncrement ? 32 : 1;
			}
			break;
	}

	return res;
}

void ppu2C02::cpuWrite(uint16_t addr, uint8_t data) {
	switch(addr) {
		case 0x2000: {
			bool old = Control.enableNMI;
			Control.reg = data;
			tramAddr.nametableX = Control.nametableX;
			tramAddr.nametableY = Control.nametableY;

			if(!old && Control.enableNMI && Status.VerticalBlank && (scanlineY != -1 || scanlineX < 1)) {
				nmi = 2;
			}
			break;
		}
		case 0x2001:
			Mask.reg = data;
			break;
		case 0x2003:
			oamAddr = data;
			break;
		case 0x2004: // Ignore during rendering
			pOAM[oamAddr] = data;
			oamAddr++;
			break;
		case 0x2005:
			if(writeState) {
				tramAddr.fineY = data & 7;
				tramAddr.coarseY = data >> 3;
			} else {
				fineX = data & 7;
				tramAddr.coarseX = data >> 3;
			}
			writeState = !writeState;
			break;
		case 0x2006:
			if(writeState) {
				tramAddr.reg = (tramAddr.reg & 0xFF00) | data;
				vramAddr = tramAddr;
				ppuRead(vramAddr.reg);
			} else {
				tramAddr.reg = (tramAddr.reg & 0x00FF) | ((data & 0x3F) << 8);
			}
			writeState = !writeState;
			break;
		case 0x2007:
			ppuWrite(vramAddr.reg, data);

			vramAddr.reg += Control.vramIncrement ? 32 : 1;
			break;
		// case 0x4014: // Handled by bus
	}
}

uint8_t ppu2C02::ppuRead(uint16_t addr, bool readOnly) {
	uint8_t data = 0;

	addr &= 0x3FFF;
	if(cartridge->ppuRead(addr, data, readOnly)) {
		// Patten tables: 0x000 - 0x1FFF
	} else {
		return getRef(addr);
	}

	return data;
}

void ppu2C02::ppuWrite(uint16_t addr, uint8_t data) {
	addr &= 0x3FFF;
	if(cartridge->ppuWrite(addr, data)) {
		// 0x000 - 0x1FFF
	} else {
		getRef(addr) = data;
	}
}

void ppu2C02::LoadBackgroundShifters() {
	bgShifterPatternLo = (bgShifterPatternLo & 0xFF00) | bgNextTileLsb;
	bgShifterPatternHi = (bgShifterPatternHi & 0xFF00) | bgNextTileMsb;

	bgShifterAttribLo = (bgShifterAttribLo & 0xFF00) | ((bgNextTileAttrib & 0b01) ? 0xFF : 0x00);
	bgShifterAttribHi = (bgShifterAttribHi & 0xFF00) | ((bgNextTileAttrib & 0b10) ? 0xFF : 0x00);
}

uint8_t& ppu2C02::getRef(uint16_t addr) {
	if(addr <= 0x1FFF) {
		return chrRAM[addr]; // in case cartridge doesn't have rom
	}
	if(addr <= 0x3EFF) {
		// 0x2000 - 0x3EFF
		switch(cartridge->mirror) {
			case MirrorMode::Horizontal:
				if(addr < 0x2800) {
					addr &= 0x3ff;
				} else {
					addr = (addr & 0x3ff) + 0x400;
				}
				break;
			case MirrorMode::Vertical:
				addr &= 0x7ff;
				break;
			case MirrorMode::OnescreenLo:
				addr &= 0x3ff;
				break;
			case MirrorMode::OnescreenHi:
				addr &= 0x3ff + 0x400;
				break;
			case MirrorMode::FourScreen:
				addr &= 0xFFF;
				break;
		}

		return vram[addr];
	}
	if(addr <= 0x3FFF) {
		// 0x3F00 - 0x3FFF
		addr &= 0x001F;
		switch(addr) {
			case 0x10:
				addr = 0x0000;
				break;
			case 0x14:
				addr = 0x0004;
				break;
			case 0x18:
				addr = 0x0008;
				break;
			case 0x1C:
				addr = 0x000C;
				break;
		}
		return palettes[addr];
	}

	throw std::logic_error("unreachable");
}

Color ppu2C02::GetPaletteColor(uint8_t palette, uint8_t pixel) const {
	uint8_t addr = ((palette << 2) + pixel) & 0x1F;

	switch(addr) {
		case 0x10:
			addr = 0x0000;
			break;
		case 0x14:
			addr = 0x0004;
			break;
		case 0x18:
			addr = 0x0008;
			break;
		case 0x1C:
			addr = 0x000C;
			break;
	}

	return colors[palettes[addr] & 0x3F];
}
