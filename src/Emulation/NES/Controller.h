#pragma once
#include <cstdint>

namespace Nes {

class Controller {
  public:
	virtual ~Controller() = default;

	virtual void CpuWrite(uint16_t addr, uint8_t data) = 0;
	virtual uint8_t CpuRead(uint16_t addr, bool readOnly) = 0;
};

}