#include "TasController.h"

namespace Nes {

TasController::TasController(std::vector<uint8_t> inputs) : inputs(inputs) {
	inputIndex = 1;
}

void TasController::CpuWrite(uint16_t addr, uint8_t data) {
	if(ShiftStrobe) {
		ControllerLatch = inputs[inputIndex];
	}
	ShiftStrobe = data & 1;
}

uint8_t TasController::CpuRead(uint16_t addr, bool readOnly) {
	if(ShiftStrobe) {
		ControllerLatch = inputs[inputIndex];
	}

	auto ret = ControllerLatch & 1;
	if(!readOnly) {
		ControllerLatch >>= 1;
	}
	return ret;
}

void TasController::Frame() {
	inputIndex++;
}

std::string TasController::GetInput() {
	auto a = inputs[inputIndex];
	std::string inputs = "........";

	char chars[] = {'A', 'B', 'S', 'T', 'U', 'D', 'L', 'R'};

	for(int i = 0; i < 8; ++i) {
		if(a & (1 << i)) inputs[7 - i] = chars[i];
	}

	return inputs;
}

}
