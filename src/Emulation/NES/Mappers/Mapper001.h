#pragma once
#include "../../../MemoryMapped.h"
#include "Mapper.h"

namespace Nes {

class Mapper001 final : public Mapper {
  private:
	bool lastWrite = false;

	uint8_t Control = 0b01100;
	uint8_t shiftRegister = 0b100000;

	uint8_t chrBank0 = 0, chrBank1 = 0, prgBank = 0;

	std::array<uint32_t, 2> prgBankOffset;
	std::array<uint32_t, 2> chrBankOffset { 0, 0x1000 };

	bool ramEnable = true;
	uint8_t* prgRam = nullptr;

	std::unique_ptr<MemoryMapped> file;

  public:
	Mapper001(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr);
	~Mapper001() override;

	int cpuRead(uint16_t addr, uint8_t& data) override;
	bool cpuWrite(uint16_t addr, uint8_t data) override;
	uint8_t ppuRead(uint16_t addr, bool readOnly) override;

	void SaveState(nlohmann::json& saver) const override;
	void LoadState(const nlohmann::json& saver) override;

	void HardReset() override {
		Mapper::HardReset();
		lastWrite = false;
		Control = 0b01100;
		shiftRegister = 0b100000;
		chrBank0 = 0;
		chrBank1 = 0;
		prgBank = 0;
		prgBankOffset = { 0, (uint32_t)prg.size() - 0x4000 };
		chrBankOffset = { 0, 0x1000 };
		ramEnable = true;
		if(!file) { // clear non save ram
			std::memset(prgRam, 0, 0x2000);
		}
	}
	void MapSaveRam(const std::string& path) override;
};

}
