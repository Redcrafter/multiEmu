#pragma once
#include "../ICore.h"
#include "Gameboy.h"
#include "Windows/ppuWindow.h"

namespace Gameboy {

class Core final : public ICore {
  private:
	RenderImage texture;

	md5 romHash;

	int currentTrack = -1;
	int selectedTrack = -1;
	Mode mode;

  public:
	Gameboy gameboy;
	ppuWindow _ppuWindow;

	Core();
	~Core() override = default;

	std::string GetName() override { return "Gameboy"; }
	ImVec2 GetSize() override { return { 160, 144 }; } 
	md5 GetRomHash() override { return romHash; }

	std::vector<MemoryDomain> GetMemoryDomains() override;
	void WriteMemory(int domain, size_t address, uint8_t val) override;
	uint8_t ReadMemory(int domain, size_t address) override;

	void Draw() override;
	void DrawMenuBar(bool& menuOpen) override;

	void SaveState(saver& saver) override { gameboy.SaveState(saver); }
	void LoadState(saver& saver) override { gameboy.LoadState(saver); }

	void LoadRom(const std::string& path) override;

	void Reset() override;
	void HardReset() override {
		Reset(); // no hard reset
	}
	void Update() override;
};

}
