#pragma once
#include <cassert>
#include <cstdint>
#include <stdexcept>

#include "../../../md5.h"
#include "../../../saver.h"

namespace Nes {

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

	bool hasSram = false;

  public:
	Mapper(std::vector<uint8_t> prg, std::vector<uint8_t> chr) : prg(std::move(prg)), chr(std::move(chr)) {
		prgMask = this->prg.size() - 1;
		chrMask = this->chr.size() - 1;
	};
	Mapper(const Mapper&) = delete;
	virtual ~Mapper() = default;

	virtual int cpuRead(uint16_t addr, uint8_t& data) = 0; // TODO: bool readOnly
	virtual bool cpuWrite(uint16_t addr, uint8_t data) { return false; };

	virtual bool ppuRead(uint16_t addr, uint8_t& data, bool readOnly) = 0;
	virtual bool ppuWrite(uint16_t addr, uint8_t data) { return false; };

	virtual void SaveState(saver& saver) = 0;
	virtual void LoadState(saver& saver) = 0;

	virtual void MapSaveRam(const std::string& path) {
		assert(false);
		// throw std::logic_error("Saveram not supported");
	};

	virtual void CpuClock() {};
};

}
