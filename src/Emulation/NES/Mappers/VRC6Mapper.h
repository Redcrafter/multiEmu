#pragma once
#include "Mapper.h"

namespace Nes {

class VRC6Mapper final : public Mapper {
  private:
	int prgBanks;
	int chrBanks;

	bool swap;
	std::array<uint32_t, 4> prgBankOffset;

	std::array<uint8_t, 8> chr_banks_1k;

	uint8_t PPUBankingMode = 0;
	bool chrA10replace = false;
	bool NTROM = false;

	bool ramEnable = false;
	std::array<uint8_t, 0x2000> prgRam;

	bool irq_enabled = false;
	bool irq_mode = false;
	// bool irq_pending;
	bool irq_autoen = false;
	uint8_t irq_reload = 0;
	uint8_t irq_counter = 0;
	int16_t irq_prescaler = 0;

  public:
	VRC6Mapper(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr, bool swap);
	~VRC6Mapper() override = default;

	int cpuRead(uint16_t addr, uint8_t& data) override;
	bool cpuWrite(uint16_t addr, uint8_t data) override;

	uint8_t ppuRead(uint16_t addr, bool readOnly) override;
	void ppuWrite(uint16_t addr, uint8_t data) override;

	void SaveState(nlohmann::json& saver) const override {}
	void LoadState(const nlohmann::json& saver) override {}

	void HardReset() override;
	void CpuClock() override;

  private:
	uint32_t mapaddr(uint16_t addr);
};

}
