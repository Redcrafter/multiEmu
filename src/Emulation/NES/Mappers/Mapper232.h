#pragma once
#include "Mapper.h"

namespace Nes {

class Mapper232 final : public Mapper {
  private:
	std::array<uint8_t, 2> prgBanks { 0, 0xFF };

  public:
	Mapper232(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr);
	~Mapper232() override = default;

	int cpuRead(uint16_t addr, uint8_t& data) override;
	bool cpuWrite(uint16_t addr, uint8_t data) override;

	uint8_t ppuRead(uint16_t addr, bool readOnly) override;

	void SaveState(nlohmann::json& saver) const override;
	void LoadState(const nlohmann::json& saver) override;

	void HardReset() override {
		Mapper::HardReset();
		prgBanks = { 0, 0xFF };
	}
};

}
