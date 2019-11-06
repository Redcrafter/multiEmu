#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>

#include "../Mappers/Mapper.h"
#include "../saver.h"
#include "../md5.h"

enum class MirrorMode : uint8_t {
	Horizontal,
	Vertical,
	OnescreenLo,
	OnescreenHi
};

class Cartridge {
public:
	md5* hash;

	uint8_t mapperId;
	std::shared_ptr<Mapper> mapper;
private:
	uint8_t prgBanks = 0;
	uint8_t chrBanks = 0;
	
	std::vector<uint8_t> prgRom;
	std::vector<uint8_t> chrRom;
public:
	Cartridge(std::string path);
	~Cartridge();

	MirrorMode GetMirror() const { return mapper->mirror; }
	bool GetIrq() const { return mapper->Irq; }

	// Communication with cpu bus
	bool cpuRead(uint16_t addr, uint8_t& data);
	bool cpuWrite(uint16_t addr, uint8_t data);

	// Communication with ppu bus
	bool ppuRead(uint16_t addr, uint8_t& data);
	bool ppuWrite(uint16_t addr, uint8_t data);

	void SaveState(saver& saver) { mapper->SaveState(saver); }
	void LoadState(saver& saver) { mapper->LoadState(saver); }
};
