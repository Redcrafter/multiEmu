#include "StandardController.h"
#include "../Input.h"

StandardController::StandardController(int number): number(number), ControllerLatch(0), ShiftStrobe(false) {
}

void StandardController::CpuWrite(uint16_t addr, uint8_t data) {
	UpdateState(); // Write before resetting
	ShiftStrobe = data & 1;
}

uint8_t StandardController::CpuRead(uint16_t addr) {
	UpdateState();
	auto ret = ControllerLatch & 1;
	ControllerLatch >>= 1;
	return ret;
}

void StandardController::UpdateState() {
	if(ShiftStrobe) {
		ControllerLatch = Input::GetController(number);
	}
}
