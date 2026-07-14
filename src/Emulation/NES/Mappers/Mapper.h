#pragma once
#include <array>
#include <cassert>
#include <cstdint>
#include <stdexcept>

#include <nlohmann/json.hpp>

#include "../../../md5.h"

namespace Nes {

enum class MirrorMode : uint8_t {
	Horizontal,
	Vertical,
	OnescreenLo,
	OnescreenHi,
	FourScreen
};

inline uint16_t mapMirrorAddress(const MirrorMode mode, uint16_t addr) {
	switch(mode) {
		case MirrorMode::Horizontal:
			if(addr < 0x2800) {
				addr &= 0x3ff;
			} else {
				addr = (addr & 0x3ff) + 0x400;
			}
			break;
		case MirrorMode::Vertical:
			addr &= 0x7ff;
			break;
		case MirrorMode::OnescreenLo:
			addr &= 0x3ff;
			break;
		case MirrorMode::OnescreenHi:
			addr &= 0x3ff + 0x400;
			break;
		case MirrorMode::FourScreen:
			addr &= 0xFFF;
			break;
	}

	return addr;
}

class Mapper {
  public:
	std::array<uint8_t, 0x1000> vram {};
	std::array<uint8_t, 0x2000> chrRam {};

	std::vector<uint8_t> prg;
	std::vector<uint8_t> chr;

	size_t prgMask;
	size_t chrMask;

	bool Irq = false;

	MirrorMode mirror = MirrorMode::Horizontal;
	md5 hash;

  public:
	Mapper(const std::vector<uint8_t>& prg, const std::vector<uint8_t>& chr) : prg(prg), chr(chr) {
		prgMask = this->prg.size() - 1;
		chrMask = this->chr.size() - 1;
	}
	Mapper(const Mapper&) = delete;
	virtual ~Mapper() = default;

	virtual int cpuRead(uint16_t addr, uint8_t& data) = 0; // TODO: bool readOnly
	virtual bool cpuWrite(uint16_t addr, uint8_t data) { return false; }

	virtual uint8_t ppuRead(uint16_t addr, bool readOnly) = 0;
	virtual void ppuWrite(uint16_t addr, uint8_t data) {
		if(addr < 0x2000) { // 0x0000 - 0x1FFF
			if(chr.empty())
				chrRam[addr] = data;
		} else { // 0x2000 - 0x3EFF
			vram[mapMirrorAddress(mirror, addr)] = data;
		}
	}

	virtual void SaveState(nlohmann::json& saver) const = 0;
	virtual void LoadState(const nlohmann::json& saver) = 0;

	virtual void MapSaveRam(const std::string& path) {
		throw std::logic_error("Mapper does not support Saveram");
	}

	virtual void CpuClock() {}
};

}
