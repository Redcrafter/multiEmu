#include "imgui_pattern_tables.h"
#include <imgui.h>

static uint8_t FlipByte(uint8_t b) {
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	return b;
}


PatternTables::PatternTables(std::string title) : Title(std::move(title)), image(256, 160) {
	// 256x128 pattern tables
	// two pattern tables 128x128 side by side
	// 256x32 sprites 
	// 2 rows of 32 sprites one sprite is 8x16
}

void PatternTables::DrawWindow() {
	if(!open || !ppu) {
		return;
	}

	if(ImGui::Begin(Title.c_str(), &open, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_AlwaysAutoResize)) {
		if(ImGui::BeginMenuBar()) {
			ImGui::EndMenuBar();
		}

		DrawPatternTable();
		for(size_t i = 0; i < 64; i++) {
			RenderSprite(i);
		}
		image.BufferImage();

		// ImGui::BeginGroup();
		ImGui::Image(reinterpret_cast<void*>(image.GetTextureId()), ImVec2(512, 256), ImVec2(0, 0), ImVec2(1, 0.8));
		const char* test[] = {"1", "2", "3", "4", "5", "6", "7", "8"};

		ImGui::SetNextItemWidth(150);
		ImGui::Combo("Patten1 pallet", &pallet1, test, 8);

		ImGui::SameLine();
		ImGui::SetNextItemWidth(150);
		ImGui::Combo("Patten2 pallet", &pallet2, test, 8);

		ImGui::BeginGroup();
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		for(int i = 0; i < 8; ++i) {
			for(int j = 0; j < 4; ++j) {
				Color nesColor = ppu->GetPaletteColor(i, j);
				ImVec4 imColor = ImVec4(nesColor.R / 255.0, nesColor.G / 255.0, nesColor.B / 255.0, 1);

				if(j != 0) ImGui::SameLine();
				ImGui::ColorButton("Test", imColor);
			}
		}
		ImGui::PopStyleVar();
		ImGui::EndGroup();

		ImGui::SameLine();
		ImGui::BeginGroup();
		const auto height = ppu->Control.spriteSize ? 16 : 8;
		const float w = 8 / 256.0;
		const float h = height * (1 / 160.0);

		int hoveredId = -1;


		for(int i = 0; i < 64; i++) {
			if(i % 16 != 0) {
				ImGui::SameLine();
			}

			float x = (i % 32) * w;
			float y = 0.8f + (i / 32) * (16 / 160.0);

			ImGui::Image(reinterpret_cast<void*>(image.GetTextureId()), ImVec2(16, height * 2), ImVec2(x, y), ImVec2(x + w, y + h));

			if(ImGui::IsItemHovered()) {
				hoveredId = i;
			}
		}

		if(hoveredId != -1) {
			auto item = ppu->oam[hoveredId];
			// TODO: make this look better
			ImGui::Text("Number: %02i \t Tile: %x\nX: %03i \t\t Color:%i\nY: %03i \t\t Flags: ", hoveredId, 0, item.x, item.Attributes.Palette + 4, item.y);
		}
		ImGui::EndGroup();
	}

	ImGui::End();
}

void PatternTables::RenderSprite(int id) {
	const Sprite sprite = ppu->oam[id];
	int x = (id % 32) * 8;
	int y = (id / 32) * 16;

	const auto spriteSize = ppu->Control.spriteSize;
	const auto patternSprite = ppu->Control.patternSprite;
	// draw sprites
	const auto height = spriteSize ? 16 : 8;


	for(int y1 = 0; y1 < height; y1++) {
		uint16_t addr;

		if(!spriteSize) {
			// 8x8
			addr = (patternSprite << 12) | (sprite.id << 4);
		} else {
			// 8x16
			addr = (sprite.id & 1) << 12;

			if((y1 < 8) != sprite.Attributes.FlipVertical) {
				addr |= (sprite.id & 0xFE) << 4;
			} else {
				addr |= ((sprite.id & 0xFE) + 1) << 4;
			}
		}

		if(sprite.Attributes.FlipVertical) {
			addr |= (7 - y1) & 0x7;
		} else {
			addr |= y1 & 0x7;
		}

		uint8_t lo = ppu->ppuRead(addr, true);
		uint8_t hi = ppu->ppuRead(addr + 8, true);

		if(sprite.Attributes.FlipHorizontal) {
			lo = FlipByte(lo);
			hi = FlipByte(hi);
		}

		for(int x1 = 0; x1 < 8; ++x1) {
			uint8_t p0_pixel = (lo & 0x80) > 0;
			uint8_t p1_pixel = (hi & 0x80) > 0;

			lo <<= 1;
			hi <<= 1;

			uint8_t fgPixel = (p1_pixel << 1) | p0_pixel;
			uint8_t fgPalette = sprite.Attributes.Palette + 4;

			image.SetPixel(x + x1, 128 + y + y1, ppu->GetPaletteColor(fgPalette, fgPixel));
		}
	}
}

void PatternTables::DrawPatternTable() {
	for(uint16_t nTileY = 0; nTileY < 16; nTileY++) {
		for(uint16_t nTileX = 0; nTileX < 16; nTileX++) {
			uint16_t nOffset = (nTileY * 16 + nTileX) * 16;

			for(uint16_t row = 0; row < 8; row++) {
				uint8_t tile_lsb1 = ppu->ppuRead(nOffset + row + 0, true);
				uint8_t tile_msb1 = ppu->ppuRead(nOffset + row + 8, true);

				uint8_t tile_lsb2 = ppu->ppuRead(0x1000 + nOffset + row + 0, true);
				uint8_t tile_msb2 = ppu->ppuRead(0x1000 + nOffset + row + 8, true);

				for(uint16_t col = 0; col < 8; col++) {
					uint8_t pixel1 = (tile_lsb1 & 1) | ((tile_msb1 & 1) << 1);
					uint8_t pixel2 = (tile_lsb2 & 1) | ((tile_msb2 & 1) << 1);

					tile_lsb1 >>= 1;
					tile_msb1 >>= 1;

					tile_lsb2 >>= 1;
					tile_msb2 >>= 1;

					auto color1 = ppu->GetPaletteColor(pallet1, pixel1);
					auto color2 = ppu->GetPaletteColor(pallet2, pixel2);
					auto x = nTileX * 8 + (7 - col);
					auto y = nTileY * 8 + row;

					image.SetPixel(x, y, color1);
					image.SetPixel(x + 128, y, color2);
				}
			}
		}
	}
}
