#pragma once
#include <cstdint>

#include "../saver.h"

class Cartridge;
enum class MirrorMode: uint8_t;

class Mapper {
public:
	Mapper(uint8_t prgBanks, uint8_t chrBanks) : prgBanks(prgBanks), chrBanks(chrBanks) { };

	virtual int cpuMapRead(uint16_t addr, uint32_t& mapped) = 0;
	virtual bool cpuMapWrite(uint16_t addr, uint8_t data) = 0;
	virtual bool ppuMapRead(uint16_t addr, uint32_t& mapped, bool readOnly) = 0;
	virtual bool ppuMapWrite(uint16_t addr, uint8_t data) = 0;

	// TODO: virtual uint8_t* GetSaveRam() = 0;
	virtual void SaveState(saver& saver) = 0;
	virtual void LoadState(saver& saver) = 0;
protected:
	bool Irq = false;
	
	uint8_t prgBanks = 0;
	uint8_t chrBanks = 0;
	MirrorMode mirror;

	friend class Cartridge;
};