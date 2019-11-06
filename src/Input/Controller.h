#pragma once
#include <cstdint>
#include <GLFW/glfw3.h>

class Controller {
public:
	virtual void CpuWrite(uint16_t addr, uint8_t data) = 0;
	virtual uint8_t CpuRead(uint16_t addr) = 0;
};
