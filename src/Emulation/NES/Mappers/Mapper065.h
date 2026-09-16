#pragma once
#include "Mapper.h"

namespace Nes {

class Mapper065 final : public Mapper {
  private:
	std::array<uint32_t, 4> prgBankOffset { 0, 1, 0xFE, 0xFF };
	std::array<uint32_t, 8> chrBankOffset {};

	bool irqEnable = false;
	uint16_t irqCounter = 0;
	uint16_t irqReload = 0;

  public:
	Mapper065(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr);
	~Mapper065() override = default;

	int cpuRead(uint16_t addr, uint8_t& data) override;
	bool cpuWrite(uint16_t addr, uint8_t data) override;
	uint8_t ppuRead(uint16_t addr, bool readOnly) override;

	void SaveState(nlohmann::json& saver) const override;
	void LoadState(const nlohmann::json& saver) override;

	void HardReset() override {
		Mapper::HardReset();
		prgBankOffset = { 0, 1, 0xFE, 0xFF };
		chrBankOffset.fill(0);
		irqEnable = false;
		irqCounter = 0;
		irqReload = 0;
	}
	void CpuClock() override;
};

}
