#pragma once
#include "Controller.h"

class StandardController : public Controller {
public:
	StandardController(int number);
	
	void CpuWrite(uint16_t addr, uint8_t data) override;
	uint8_t CpuRead(uint16_t addr) override;
private:
	int number;
	
	uint8_t ControllerLatch;
	bool ShiftStrobe;

	void UpdateState();
};
