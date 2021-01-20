#pragma once
#include "../ICore.h"
#include "Bus.h"

// #include "imguiWindows/imgui_tas_editor.h"
#include "Windows/pattern_tables.h"
#include "Windows/cpu_state.h"
#include "Windows/apu_window.h"
#include "Windows/disassembler.h"

class NesCore : public ICore {
    Bus emulator;
    RenderImage texture;

	// TasEditor tasEdit{"Tas Editor"};
	PatternTables tables{ "Pattern Tables" };
	CpuStateWindow cpuWindow{ "Cpu State" };
	ApuWindow apuWindow{ "Apu Visuals" };
	DisassemblerWindow disassembler{ "Disassembler" };

    std::string currentFile;
public:
	NesCore();

    std::string GetName() override {
		return "NES";
    }

    RenderImage* GetMainTexture() override {
		return &texture;
    }
	float GetPixelRatio() override {
		return 8.0 / 7.0;
    }

	md5 GetRomHash() override {
	    if(emulator.cartridge) {
			return emulator.cartridge->hash;
	    }
		return md5();
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
