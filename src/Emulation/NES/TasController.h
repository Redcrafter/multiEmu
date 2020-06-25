#pragma once
#include "Controller.h"

#include <string>
#include <vector>

class TasController : public Controller {
public:
	TasController(std::vector<uint8_t> inputs);
	
	void CpuWrite(uint16_t addr, uint8_t data) override;
	uint8_t CpuRead(uint16_t addr, bool readOnly) override;
	void Frame();

	std::string GetInput();
private:
	std::vector<uint8_t> inputs;
	uint8_t ControllerLatch = 0;
	int inputIndex = 0;
	
	bool ShiftStrobe = false;
};

