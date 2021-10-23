#pragma once
#include "../ICore.h"
#include "Bus.h"

// #include "imguiWindows/imgui_tas_editor.h"
#include "Windows/pattern_tables.h"
#include "Windows/cpu_state.h"
#include "Windows/apu_window.h"
#include "Windows/disassembler.h"

namespace Nes {

class Core : public ICore {
	Bus emulator;
	RenderImage texture;

	// TasEditor tasEdit{"Tas Editor"};
	PatternTables tables{ "Pattern Tables" };
	CpuStateWindow cpuWindow{ "Cpu State" };
	ApuWindow apuWindow{ "Apu Visuals" };
	DisassemblerWindow disassembler{ "Disassembler" };

	std::string currentFile;
public:
	Core();
	~Core() override = default;

	std::string GetName() override { return "NES"; }
	ImVec2 GetSize() override { return { 256 * (8.0 / 7.0), 240 }; } 
	md5 GetRomHash() override { return emulator.cartridge ? emulator.cartridge->hash : md5(); }

	std::vector<MemoryDomain> GetMemoryDomains() override;
	void WriteMemory(int domain, size_t address, uint8_t val) override;
	uint8_t ReadMemory(int domain, size_t address) override;

	void Draw() override;
	void DrawMenuBar(bool& menuOpen) override;

	void SaveState(saver& saver) override;
	void LoadState(saver& saver) override;

	void LoadRom(const std::string& path) override;
	void Reset() override;
	void HardReset() override;
	void Update() override;
};

}
