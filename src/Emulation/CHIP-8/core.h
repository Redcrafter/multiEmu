#pragma once
#include "../ICore.h"

#include <fstream>

#include "chip8.h"
#include "disassembler.h"

namespace Chip8 {

class Core : public ICore {
	Chip8 emulator;
	RenderImage texture;

	std::string currentFile;
	md5 currentFileHash {};

	int beepFrames = 12;
	int beepPos = 0;

	DisassemblerWindow disassembler;

  public:
	Core();
	~Core() override = default;

	std::string GetName() override { return "Chip-8"; }
	ImVec2 GetSize() override { return { 64, 32 }; } 
	md5 GetRomHash() override { return currentFileHash; }

	std::vector<MemoryDomain> GetMemoryDomains() override;
	void WriteMemory(int domain, size_t address, uint8_t val) override;
	uint8_t ReadMemory(int domain, size_t address) override;

	void DrawMenuBar(bool& menuOpen) override;
	void Draw() override {
		DrawTextureWindow(texture);
		disassembler.DrawWindow();
	}

	void SaveState(saver& saver) override;
	void LoadState(saver& saver) override;

	void LoadRom(const std::string& path) override {
		emulator.Reset();
		emulator.LoadRom(path);
		disassembler.Update();

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

}