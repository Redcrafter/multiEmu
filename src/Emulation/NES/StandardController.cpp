#include "StandardController.h"
#include "../../Input.h"

static Input::Mapper inputMapper ("Nes", {
	{ "Controller1 A",      0,  { GLFW_KEY_A,     0 } },
	{ "Controller1 B",      1,  { GLFW_KEY_B,     0 } },
	{ "Controller1 Start",  2,  { GLFW_KEY_S,     0 } },
	{ "Controller1 Select", 3,  { GLFW_KEY_ENTER, 0 } },
	{ "Controller1 Up",     4,  { GLFW_KEY_UP,    0 } },
	{ "Controller1 Down",   5,  { GLFW_KEY_DOWN,  0 } },
	{ "Controller1 Left",   6,  { GLFW_KEY_LEFT,  0 } },
	{ "Controller1 Right",  7,  { GLFW_KEY_RIGHT, 0 } },

	{ "Controller2 A",      8,  { 0, 0 } },
	{ "Controller2 B",      9,  { 0, 0 } },
	{ "Controller2 Start",  10, { 0, 0 } },
	{ "Controller2 Select", 11, { 0, 0 } },
	{ "Controller2 Up",     12, { 0, 0 } },
	{ "Controller2 Down",   13, { 0, 0 } },
	{ "Controller2 Left",   14, { 0, 0 } },
	{ "Controller2 Right",  15, { 0, 0 } },
});

namespace Nes {

static uint8_t GetController(int number) {
	int offset = number * 8;
	uint8_t val = 0;

	for(int i = 0; i < 8; ++i) {
		val |= inputMapper.GetKey(offset + i) << i;
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
