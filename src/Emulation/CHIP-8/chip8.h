#pragma once

#include <fstream>
#include <string>

#include "../ICore.h"
#include "Input.h"

struct Chip8 {
    uint8_t V[16];
    uint8_t memory[0x1000];

    uint16_t I, PC;
    uint8_t delay_timer, sound_timer;

    uint8_t SP;
    uint16_t stack[0x100];

    uint8_t gfx[64 * 32];
    // RenderImage* image;
public:
    Chip8();

    void LoadRom(const std::string& path);

    void Reset();
    void Clock();
};

class Chip8Core : public ICore {
    Chip8 emulator;
	RenderImage texture;

	std::string currentFile;
	md5 currentFileHash;
public:
	Chip8Core();

	std::string GetName() override {
		return "Chip-8";
    }

	RenderImage* GetMainTexture() override {
		return &texture;
    }

	md5 GetRomHash() override {
		return md5{ };
	}

	std::vector<MemoryDomain> GetMemoryDomains() override;
	void WriteMemory(int domain, size_t address, uint8_t val) override;
	uint8_t ReadMemory(int domain, size_t address) override;

	void DrawMenuBar(bool& menuOpen) override { }
	void DrawWindows() override { }

	void SaveState(saver& saver) override;
	void LoadState(saver& saver) override;

	void LoadRom(const std::string& path) override {
		emulator.Reset();
		emulator.LoadRom(path);

		std::ifstream file(path, std::ios::binary);
		currentFileHash = md5(file);

		currentFile = path;
    }

	void Reset() override {
		emulator.Reset();
		emulator.LoadRom(currentFile);
	}
	void HardReset() override {
		emulator.Reset();
		emulator.LoadRom(currentFile);
	}
	void Update() override;
};
