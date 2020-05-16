#pragma once
#include <cstdint>

class Controller {
public:
	virtual void CpuWrite(uint16_t addr, uint8_t data) = 0;
	virtual uint8_t CpuRead(uint16_t addr, bool readOnly) = 0;
};
