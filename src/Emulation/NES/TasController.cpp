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

	if(a & (1 << 0))
		inputs[7] = 'A';
	if(a & (1 << 1))
		inputs[6] = 'B';
	if(a & (1 << 2))
		inputs[5] = 'S';
	if(a & (1 << 3))
		inputs[4] = 'T';
	if(a & (1 << 4))
		inputs[3] = 'U';
	if(a & (1 << 5))
		inputs[2] = 'D';
	if(a & (1 << 6))
		inputs[1] = 'L';
	if(a & (1 << 7))
		inputs[0] = 'R';

	return inputs;
}

}
