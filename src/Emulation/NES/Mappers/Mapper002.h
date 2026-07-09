#pragma once
#include "Mapper.h"

namespace Nes {

class Mapper002 : public Mapper {
  public:
	Mapper002(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr);
	~Mapper002() override = default;

	int cpuRead(uint16_t addr, uint8_t& data) override;
	bool cpuWrite(uint16_t addr, uint8_t data) override;
	bool ppuRead(uint16_t addr, uint8_t& data, bool readOnly) override;

	void SaveState(nlohmann::json& saver) const override;
	void LoadState(const nlohmann::json& saver) override;

  private:
	uint8_t selectedBank = 0;
};

}
