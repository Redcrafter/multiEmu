#include "Input.h"

#include "fs.h"
#include <fstream>
#include <cassert>

int Input::keys[16] { 0 };
std::map<int, Action> Input::keyMap;

void Input::LoadKeyMap() {
	if(fs::exists("./keymap.txt")) {
		std::ifstream stream("./keymap.txt");

		throw std::logic_error("Not implemented");
	} else {
		printf("keymap.txt not found. Using fallback.");

		keyMap[GLFW_KEY_A] = Action::Controller1A;
		keyMap[GLFW_KEY_B] = Action::Controller1B;
		keyMap[GLFW_KEY_S] = Action::Controller1Start;
		keyMap[GLFW_KEY_ENTER] = Action::Controller1Select;

		keyMap[GLFW_KEY_UP] = Action::Controller1Up;
		keyMap[GLFW_KEY_DOWN] = Action::Controller1Down;
		keyMap[GLFW_KEY_LEFT] = Action::Controller1Left;
		keyMap[GLFW_KEY_RIGHT] = Action::Controller1Right;
		

		keyMap[GLFW_KEY_Q] = Action::Speedup;
		// ChangePallet unmapped
		keyMap[GLFW_KEY_F] = Action::Step;
		keyMap[GLFW_KEY_G] = Action::ResumeRun;

		keyMap[GLFW_KEY_R] = Action::Reset;

		keyMap[GLFW_KEY_K] = Action::SaveState;
		keyMap[GLFW_KEY_L] = Action::LoadState;
		keyMap[GLFW_KEY_KP_ADD] = Action::SelectNextState;
		keyMap[GLFW_KEY_KP_SUBTRACT] = Action::SelectLastState;
	}
}

void Input::OnKey(int key, int scancode, int action, int mods) {
	Action mapped;
	if(!TryGetAction(key, mapped)) {
		return;
	}
	
	switch(action) {
		case GLFW_PRESS:
			if(static_cast<int>(mapped) < 16) {
				keys[static_cast<int>(mapped)] = 1;
			}
			break;
		case GLFW_RELEASE:
			if(static_cast<int>(mapped) < 16) {
				keys[static_cast<int>(mapped)] = 0;
			}
			break;
	}
}

bool Input::TryGetAction(int key, Action& action) {
	if(!keyMap.count(key)) {
		return false;
	}
	action = keyMap.at(key);
	return true;
}

uint8_t Input::GetController(int id) {
	assert(id == 0 || id == 1);
	const int offset = id * 10;
	uint8_t val = 0;
	
	for(int i = 0; i < 8; ++i) {
		val |= keys[i + offset] << i;
	}

	return val;
}
