#pragma once
#include "../ICore.h"
#include "Gameboy.h"

namespace Gameboy {

class GameboyColorCore final : public ICore {
  private:
	RenderImage texture;

	md5 romHash;
	Gameboy gameboy;

  public:
	GameboyColorCore();
	~GameboyColorCore() override = default;

	std::string GetName() override { return "GBC"; }
	RenderImage* GetMainTexture() override { return &texture; }
	float GetPixelRatio() override { return 1; }

	md5 GetRomHash() override { return romHash; }

	std::vector<MemoryDomain> GetMemoryDomains() override;
	void WriteMemory(int domain, size_t address, uint8_t val) override;
	uint8_t ReadMemory(int domain, size_t address) override;

	void DrawMenuBar(bool& menuOpen) override {}
	void DrawWindows() override {}

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
