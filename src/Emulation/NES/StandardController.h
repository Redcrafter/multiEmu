#pragma once
#include "Controller.h"

namespace Nes {

class StandardController : public Controller {
  private:
	int number;

	uint8_t ControllerLatch = 0;
	bool ShiftStrobe = false;

  public:
	StandardController(int number);
	~StandardController() override = default;

	void CpuWrite(uint16_t addr, uint8_t data) override;
	uint8_t CpuRead(uint16_t addr, bool readOnly) override;
};

}
