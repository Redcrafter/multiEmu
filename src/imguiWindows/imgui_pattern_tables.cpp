#include "imgui_pattern_tables.h"
#include "../imgui/imgui.h"

PatternTables::PatternTables(std::string title) : Title(std::move(title)) { }

void PatternTables::Init() {
	glGenTextures(1, &textureID);

	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 128, 0, GL_BGR, GL_UNSIGNED_BYTE, imgData);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void PatternTables::DrawWindow() {
	if(!open) {
		return;
	}
	
	if(ImGui::Begin(Title.c_str(), &open, ImGuiWindowFlags_NoScrollbar)) {
		if(ImGui::BeginMenuBar()) {
			ImGui::EndMenuBar();
		}

		if(ppu) {
			DrawPatternTable();
			
			glBindTexture(GL_TEXTURE_2D, textureID);
			
			if(changed) {
				changed = false;
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 256, 128, GL_RGB, GL_UNSIGNED_BYTE, imgData);
			}
		}

		ImGui::Image((void*)textureID, ImVec2(512, 256));

		Color nesColor;
		ImVec4 imColor;

		if(ppu) {
			for(int i = 0; i < 8; ++i) {
				nesColor = ppu->GetPaletteColor(i, 0);
				imColor = ImVec4(nesColor.R / 255.0, nesColor.G / 255.0, nesColor.B / 255.0, 1);
				ImGui::ColorButton("Test", imColor);
				
				for(int j = 1; j < 4; ++j) {
					nesColor = ppu->GetPaletteColor(i, j);
					imColor = ImVec4(nesColor.R / 255.0, nesColor.G / 255.0, nesColor.B / 255.0, 1);

					ImGui::SameLine();
					ImGui::ColorButton("Test", imColor);
				}
			}
		}

		ImGui::End();
	}
}

void PatternTables::DrawPatternTable() {
	changed = true;
	
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
					
					imgData[x + y * 256] = color1;
					imgData[x + y * 256 + 128] = color2;
				}
			}
		}
	}
}
