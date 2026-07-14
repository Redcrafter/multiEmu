#pragma once
#include "../../../MemoryMapped.h"
#include "Mapper.h"

namespace Nes {

class Mapper004 final : public Mapper {
  private:
	union {
		struct {
			uint8_t bankNumber : 3;
			uint8_t unused : 2;
			bool M : 1;
			bool P : 1;
			bool C : 1;
		};

		uint8_t reg = 0; // unsepcified
	} bankSelect;

	uint8_t regs[8] { 0, 2, 4, 5, 6, 7, 0, 1 };

	std::array<uint32_t, 4> prgBankOffset;
	std::array<uint32_t, 8> chrBankOffset;

	bool reloadIrq = false;
	bool irqEnable = false;
	bool lastA12 = false;
	uint8_t irqCounter = 0;
	uint8_t irqLatch = 0;

	uint8_t* prgRam = nullptr; // 0x2000;

	std::unique_ptr<MemoryMapped> file;

	// not implemented because of compatibility issue between MMC3 and MMC6 (http://wiki.nesdev.com/w/index.php/MMC3)
	// bool ramEnable;
  public:
	Mapper004(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr);
	~Mapper004() override;

	int cpuRead(uint16_t addr, uint8_t& data) override;
	bool cpuWrite(uint16_t addr, uint8_t data) override;

	uint8_t ppuRead(uint16_t addr, bool readOnly) override;
	void ppuWrite(uint16_t addr, uint8_t data) override;

	void SaveState(nlohmann::json& saver) const override;
	void LoadState(const nlohmann::json& saver) override;

	void MapSaveRam(const std::string& path) override;

  private:
	void UpdateRegs();
};

}
