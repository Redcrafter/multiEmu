#pragma once
#include <cstdint>
#include <stdexcept>
#include <cassert>

#include "saver.h"
#include "md5.h"

enum class MirrorMode : uint8_t {
	Horizontal,
	Vertical,
	OnescreenLo,
	OnescreenHi,
	FourScreen
};

class Mapper {
public:
	std::vector<uint8_t> prg;
	std::vector<uint8_t> chr;

	size_t prgMask;
	size_t chrMask;
	
	bool Irq = false;
	MirrorMode mirror = (MirrorMode)0;
	md5 hash;
public:
	Mapper(std::vector<uint8_t> prg, std::vector<uint8_t> chr) : prg(std::move(prg)), chr(std::move(chr)) {
		prgMask = this->prg.size() - 1;
		chrMask = this->chr.size() - 1;
	};
	virtual int cpuRead(uint16_t addr, uint8_t& data) = 0; // TODO: bool readOnly
	virtual bool cpuWrite(uint16_t addr, uint8_t data) { return false; };
	
	virtual bool ppuRead(uint16_t addr, uint8_t& data, bool readOnly) = 0;
	virtual bool ppuWrite(uint16_t addr, uint8_t data) { return false; };

	// TODO: virtual uint8_t* GetSaveRam() = 0;
	virtual void SaveState(saver& saver) = 0;
	virtual void LoadState(saver& saver) = 0;

	virtual void CpuClock() { };
};