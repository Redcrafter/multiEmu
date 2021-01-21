#include "StandardController.h"
#include "Input.h"

namespace Nes {

static uint8_t GetController(int number) {
	int offset = number * 8;
	uint8_t val = 0;

	for(int i = 0; i < 8; ++i) {
		val |= Input::GetKey(offset + i) << i;
	}
	return val;
}

StandardController::StandardController(int number) : number(number) {
	// assert(number == 0 || number == 1);
}

void StandardController::CpuWrite(uint16_t addr, uint8_t data) {
	if(ShiftStrobe) {
		ControllerLatch = GetController(number);
	}
	ShiftStrobe = data & 1;
}

uint8_t StandardController::CpuRead(uint16_t addr, bool readOnly) {
	if(ShiftStrobe) {
		ControllerLatch = GetController(number);
	}
	auto ret = ControllerLatch & 1;
	if(!readOnly) {
		ControllerLatch >>= 1;
	}
	return ret;
}

}
