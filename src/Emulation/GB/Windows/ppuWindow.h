
#pragma once
#include <imgui.h>

#include "../../../math.h"
#include "../PPU.h"

namespace Gameboy {

const Color palette[] = {
	{ 0xFF, 0xFF, 0xFF },
	{ 0xAA, 0xAA, 0xAA },
	{ 0x55, 0x55, 0x55 },
	{ 0x00, 0x00, 0x00 }
};

class ppuWindow {
  private:
	bool open = false;
	const PPU& ppu;

	RenderImage image;

  public:
	ppuWindow(const PPU& ppu)
		: ppu(ppu), image(32 * 8, 12 * 8 * 2) {};

	void Open() { open = true; };
	void Close() { open = false; };

	void DrawWindow() {
		if(!open) return;

		if(ImGui::Begin("PPU", &open)) {
			auto window = ImGui::GetCurrentWindow();
			auto rect = window->ContentRegionRect;

			if(ppu.bus.gbc) {
				RenderTexture(0);
				RenderTexture(1);
				image.BufferImage();

				float scale = std::min(rect.GetWidth() / 256, rect.GetHeight() / 192);
				ImGui::Image(reinterpret_cast<void*>(image.GetTextureId()), { 256 * scale, 192 * scale });
			} else {
				RenderTexture(0);
				image.BufferImage();

				float scale = std::min(rect.GetWidth() / 256, rect.GetHeight() / 96);
				ImGui::Image(reinterpret_cast<void*>(image.GetTextureId()), { 256 * scale, 96 * scale }, {0,0}, {1, 0.5});
			}
		}

		ImGui::End();
	}

	void RenderTexture(int bank) {
		for(size_t i = 0; i < 0x80 * 3; i++) {
			auto gx = (i % 32) * 8;
			auto gy = (i / 32) * 8 + bank * 96;

			for(size_t y = 0; y < 8; y++) {
				auto addr = (i << 4) + (y << 1);

				uint16_t merged = math::interleave(ppu.VRAM[bank][addr], ppu.VRAM[bank][addr + 1]);

				for(size_t x = 0; x < 8; x++) {
					auto col = merged >> 14;
					image.SetPixel(gx + x, gy + y, palette[col]);
					merged <<= 2;
				}
			}
		}
	}
};

} // namespace Gameboy
