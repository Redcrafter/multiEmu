#include "StandardController.h"
#include "../../Input.h"

static Input::Mapper inputMapper ("Nes", {
	{ "Controller1 A",      0,  { SDL_SCANCODE_A,     0 } },
	{ "Controller1 B",      1,  { SDL_SCANCODE_B,     0 } },
	{ "Controller1 Start",  2,  { SDL_SCANCODE_S,     0 } },
	{ "Controller1 Select", 3,  { SDL_SCANCODE_RETURN, 0 } },
	{ "Controller1 Up",     4,  { SDL_SCANCODE_UP,    0 } },
	{ "Controller1 Down",   5,  { SDL_SCANCODE_DOWN,  0 } },
	{ "Controller1 Left",   6,  { SDL_SCANCODE_LEFT,  0 } },
	{ "Controller1 Right",  7,  { SDL_SCANCODE_RIGHT, 0 } },

	{ "Controller2 A",      8,  { SDL_SCANCODE_UNKNOWN, 0 } },
	{ "Controller2 B",      9,  { SDL_SCANCODE_UNKNOWN, 0 } },
	{ "Controller2 Start",  10, { SDL_SCANCODE_UNKNOWN, 0 } },
	{ "Controller2 Select", 11, { SDL_SCANCODE_UNKNOWN, 0 } },
	{ "Controller2 Up",     12, { SDL_SCANCODE_UNKNOWN, 0 } },
	{ "Controller2 Down",   13, { SDL_SCANCODE_UNKNOWN, 0 } },
	{ "Controller2 Left",   14, { SDL_SCANCODE_UNKNOWN, 0 } },
	{ "Controller2 Right",  15, { SDL_SCANCODE_UNKNOWN, 0 } },
});

namespace Nes {

static uint8_t GetController(int number) {
	int offset = number * 8;
	uint8_t val = 0;

	for(int i = 0; i < 8; ++i) {
		val |= inputMapper.GetKey(offset + i) << i;
	}
	return ~val;
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
	return !ret;
}

}
