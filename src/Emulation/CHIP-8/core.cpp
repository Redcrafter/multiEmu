#include "core.h"

#define _USE_MATH_DEFINES
#include <math.h>

#include <imgui.h>

#include "../../Input.h"
#include "../../audio.h"

namespace Chip8 {

Core::Core() : texture(64, 32), disassembler(emulator) {
	Input::SetMapper(Input::InputMapper("Chip-8", {
        {"0", 0,  { { GLFW_KEY_1, 0 } } },
        {"1", 1,  { { GLFW_KEY_2, 0 } } },
        {"2", 2,  { { GLFW_KEY_3, 0 } } },
        {"3", 3,  { { GLFW_KEY_4, 0 } } },
        {"4", 4,  { { GLFW_KEY_Q, 0 } } },
        {"5", 5,  { { GLFW_KEY_W, 0 } } },
        {"6", 6,  { { GLFW_KEY_E, 0 } } },
        {"7", 7,  { { GLFW_KEY_R, 0 } } },
        {"8", 8,  { { GLFW_KEY_A, 0 } } },
        {"9", 9,  { { GLFW_KEY_S, 0 } } },
        {"A", 10, { { GLFW_KEY_D, 0 } } },
        {"B", 11, { { GLFW_KEY_F, 0 } } },
        {"C", 12, { { GLFW_KEY_Y, 0 } } },
        {"D", 13, { { GLFW_KEY_X, 0 } } },
        {"E", 14, { { GLFW_KEY_C, 0 } } },
        {"F", 15, { { GLFW_KEY_V, 0 } } },
	}));
}

std::vector<MemoryDomain> Core::GetMemoryDomains() {
	return {
		MemoryDomain { 0, "RAM", sizeof(emulator.memory) },
		MemoryDomain { 1, "Stack", sizeof(emulator.stack) }
	};
}

void Core::WriteMemory(int domain, size_t address, uint8_t val) {
	switch(domain) {
		case 0: emulator.memory[address] = val; break;
		case 1:
			reinterpret_cast<uint8_t*>(&emulator.stack)[address] = val;
			break;
	}
}

uint8_t Core::ReadMemory(int domain, size_t address) {
	switch(domain) {
		case 0: return emulator.memory[address];
		case 1: return reinterpret_cast<uint8_t*>(&emulator.stack)[address];
	}
	return 0;
}

void Core::DrawMenuBar(bool& menuOpen) {
	if(ImGui::BeginMenu("CHIP-8")) {
		menuOpen = true;

		if(ImGui::MenuItem("Disassembler")) {
			disassembler.Open();
		}

		ImGui::EndMenu();
	}
}

void Core::SaveState(saver& saver) { saver << emulator; }
void Core::LoadState(saver& saver) { saver >> emulator; }

void Core::Update() {
	// target clock rate 540hz/60 = 9
	for(int i = 0; i < 9; i++) {
		emulator.Clock();
	}
	if(emulator.delay_timer > 0) {
		emulator.delay_timer--;
	}

	if(emulator.sound_timer > 0) {
		emulator.sound_timer--;
		if(emulator.sound_timer == 0) {
			// beep should play for 200 ms
			// 200ms / 16.66ms = 12 frames
			beepFrames = 12;
			beepPos = 0;
		}
	}

	if(beepFrames > 0) {
		// beep plays at 800hz
		const auto frac = (M_PI * 800 * 2) / (735 * 60);
		for(int i = 0; i < 735; ++i) {
			Audio::PushSample(std::sin(beepPos * frac) * 0.1);
			beepPos++;
		}
		beepFrames--;
	}

	Color black { 0, 0, 0 };
	Color white { 255, 255, 255 };

	for(int y = 0; y < 32; ++y) {
		for(int x = 0; x < 64; ++x) {
			texture.SetPixel(x, y, emulator.gfx[x + y * 64] ? white : black);
		}
	}
}

}