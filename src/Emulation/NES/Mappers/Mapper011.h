#pragma once
#include "Mapper.h"

namespace Nes {

class Mapper011 final : public Mapper {
  private:
	uint8_t prgBank = 0;
	uint8_t chrBank = 0;

  public:
	Mapper011(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr);
	~Mapper011() override = default;

	int cpuRead(uint16_t addr, uint8_t& data) override;
	bool cpuWrite(uint16_t addr, uint8_t data) override;
	uint8_t ppuRead(uint16_t addr, bool readOnly) override;

	void SaveState(nlohmann::json& saver) const override;
	void LoadState(const nlohmann::json& saver) override;

    void HardReset() override {
		Mapper::HardReset();
		prgBank = 0;
	    chrBank = 0;
	}
};

}
