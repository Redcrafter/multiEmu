#pragma once
#include "../ICore.h"
#include "Bus.h"

// #include "imguiWindows/imgui_tas_editor.h"
#include "Windows/imgui_pattern_tables.h"
#include "Windows/imgui_cpu_state.h"
#include "Windows/imgui_apu_window.h"

class NesCore : public ICore {
    Bus emulator;
    RenderImage texture;

	// TasEditor tasEdit{"Tas Editor"};
	PatternTables tables{ "Pattern Tables" };
	CpuStateWindow cpuWindow{ "Cpu State" };
	ApuWindow apuWindow{ "Apu Visuals" };

    std::string currentFile;
public:
	NesCore();

    std::string GetName() override {
		return "NES";
    }

    RenderImage* GetMainTexture() override {
		return &texture;
    }

	md5 GetRomHash() override {
	    if(emulator.cartridge) {
			return emulator.cartridge->hash;
	    }
    }

	std::vector<MemoryDomain> GetMemoryDomains() override;
	void WriteMemory(int domain, size_t address, uint8_t val) override;
	uint8_t ReadMemory(int domain, size_t address) override;

	void DrawMenuBar(bool& menuOpen) override;
	void DrawWindows() override;

	void SaveState(saver& saver) override;
	void LoadState(saver& saver) override;

    void LoadRom(const std::string& path) override;
	void Reset() override;
	void HardReset() override;
    void Update() override;
};
