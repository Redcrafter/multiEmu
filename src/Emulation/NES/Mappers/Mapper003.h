#pragma once
#include "Mapper.h"

namespace Nes {

class Mapper003 final : public Mapper {
  public:
	Mapper003(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr);
	~Mapper003() override = default;

	int cpuRead(uint16_t addr, uint8_t& data) override;
	bool cpuWrite(uint16_t addr, uint8_t data) override;
	uint8_t ppuRead(uint16_t addr, bool readOnly) override;

	void SaveState(nlohmann::json& saver) const override;
	void LoadState(const nlohmann::json& saver) override;

  private:
	uint8_t selectedBank = 0;
};

}
