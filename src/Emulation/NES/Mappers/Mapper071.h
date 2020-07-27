#pragma once
#include "Mapper.h"

class Mapper071 : public Mapper {
private:
	uint8_t prgBanks[2];
public:
	Mapper071(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr);
	
	int cpuRead(uint16_t addr, uint8_t& data) override;
	bool cpuWrite(uint16_t addr, uint8_t data) override;
	bool ppuRead(uint16_t addr, uint8_t& data, bool readOnly) override;

	void SaveState(saver& saver) override;
	void LoadState(saver& saver) override;
};
