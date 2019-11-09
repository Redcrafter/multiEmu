#include "StandardController.h"
#include "../Input.h"

StandardController::StandardController(int number): number(number), ControllerLatch(0), ShiftStrobe(false) {
}

void StandardController::CpuWrite(uint16_t addr, uint8_t data) {
	if(ShiftStrobe) {
		ControllerLatch = Input::GetController(number);
	}
	ShiftStrobe = data & 1;
}

uint8_t StandardController::CpuRead(uint16_t addr, bool readOnly) {
	if(ShiftStrobe) {
		ControllerLatch = Input::GetController(number);
	}
	auto ret = ControllerLatch & 1;
	if(!readOnly) {
		ControllerLatch >>= 1;
	}
	return ret;
}
