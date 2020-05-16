#include "Input.h"

#include <cassert>
#include <string>

#include <imgui.h>
#include <GLFW/glfw3.h>

static bool changed;
static int keys[16];
static std::map<uint64_t, Action> keyMap;
static std::map<Action, uint64_t> revKeyMap;

const char* ActionString[] = {
	"Controller1 A", "Controller1 B", "Controller1 Start", "Controller1 Select", "Controller1 Up", "Controller1 Down", "Controller1 Left", "Controller1 Right",
	"Controller2 A", "Controller2 B", "Controller2 Start", "Controller2 Select", "Controller2 Up", "Controller2 Down", "Controller2 Left", "Controller2 Right",
	"Speedup", "Step", "ResumeRun", "Reset", "SaveState", "LoadState", "SelectNextState", "SelectLastState"
};
std::map<std::string, Action> StringAction = {
	{"Controller1 A", Action::Controller1A},
	{"Controller1 B", Action::Controller1B},
	{"Controller1 Start", Action::Controller1Start},
	{"Controller1 Select", Action::Controller1Select},
	{"Controller1 Up", Action::Controller1Up},
	{"Controller1 Down", Action::Controller1Down},
	{"Controller1 Left", Action::Controller1Left},
	{"Controller1 Right", Action::Controller1Right},
	{"Controller2 A", Action::Controller2A},
	{"Controller2 B", Action::Controller2B},
	{"Controller2 Start", Action::Controller2Start},
	{"Controller2 Select", Action::Controller2Select},
	{"Controller2 Up", Action::Controller2Up},
	{"Controller2 Down", Action::Controller2Down},
	{"Controller2 Left", Action::Controller2Left},
	{"Controller2 Right", Action::Controller2Right},
	{"Speedup", Action::Speedup},
	{"Step", Action::Step},
	{"ResumeRun", Action::ResumeRun},
	{"Reset", Action::Reset},
	{"SaveState", Action::SaveState},
	{"LoadState", Action::LoadState},
	{"SelectNextState", Action::SelectNextState},
	{"SelectLastState", Action::SelectLastState}
};

const int modMask = (GLFW_MOD_SHIFT | GLFW_MOD_CONTROL | GLFW_MOD_ALT);
static int selected = -1;

void Input::OnKey(int key, int scancode, int action, int mods) {
	if(key >= GLFW_KEY_LEFT_SHIFT) {
		return;
	}

	// TODO: somehow get keys from other viewports
	if(selected != -1) {
		changed = true;

		if(revKeyMap.count((Action)selected)) {
			keyMap.erase(revKeyMap.at((Action)selected));
		}

		if(key == GLFW_KEY_BACKSPACE) {
			revKeyMap.erase((Action)selected);
			selected = -1;
			return;
		}

		Key k;
		k.Info.key = key;
		k.Info.mods = mods & modMask;
		if(keyMap.count(k.Reg)) {
			revKeyMap.erase(keyMap.at(k.Reg));
		}

		keyMap[k.Reg] = (Action)selected;
		revKeyMap[(Action)selected] = k.Reg;
		selected = -1;
	} else {
		Action mapped;

		if(!TryGetAction(key, mods, mapped)) {
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
}

void Input::Load(Json& j) {
	if(j.contains("keymap") && j["keymap"].asObject()) {
		for(const auto& item : *j["keymap"].asObject()) {
			keyMap[item.second] = StringAction[item.first];
		}
	} else {
		keyMap[GLFW_KEY_A] = Action::Controller1A;
		keyMap[GLFW_KEY_B] = Action::Controller1B;
		keyMap[GLFW_KEY_S] = Action::Controller1Start;
		keyMap[GLFW_KEY_ENTER] = Action::Controller1Select;

		keyMap[GLFW_KEY_UP] = Action::Controller1Up;
		keyMap[GLFW_KEY_DOWN] = Action::Controller1Down;
		keyMap[GLFW_KEY_LEFT] = Action::Controller1Left;
		keyMap[GLFW_KEY_RIGHT] = Action::Controller1Right;

		keyMap[GLFW_KEY_Q] = Action::Speedup;
		keyMap[GLFW_KEY_F] = Action::Step;
		keyMap[GLFW_KEY_G] = Action::ResumeRun;

		keyMap[GLFW_KEY_R] = Action::Reset;

		keyMap[GLFW_KEY_K] = Action::SaveState;
		keyMap[GLFW_KEY_L] = Action::LoadState;
		keyMap[GLFW_KEY_KP_ADD] = Action::SelectNextState;
		keyMap[GLFW_KEY_KP_SUBTRACT] = Action::SelectLastState;
	}

	for(auto map : keyMap) {
		revKeyMap[map.second] = map.first;
	}
}

void Input::Save(Json& j) {
	std::map<std::string, uint64_t> map;

	for(auto item : keyMap) {
		map[ActionString[(int)item.second]] = item.first;
	}

	j["keymap"] = map;
}

bool Input::ShowEditWindow() {
	for(int i = 0; i < 24; ++i) {
		// char* map;

		ImGui::Text("%s: ", ActionString[i]);

		ImGui::SameLine(200);

		std::string text;

		if(revKeyMap.count((Action)i)) {
			Key key;
			key.Reg = revKeyMap.at((Action)i);

			if(key.Info.mods & GLFW_MOD_SHIFT) {
				text = "shift + ";
			}
			if(key.Info.mods & GLFW_MOD_CONTROL) {
				text = "ctrl + ";
			}
			if(key.Info.mods & GLFW_MOD_ALT) {
				text = "alt + ";
			}

			switch(key.Info.key) {
				case GLFW_KEY_UP:
					text += "up";
					break;
				case GLFW_KEY_DOWN:
					text += "down";
					break;
				case GLFW_KEY_LEFT:
					text += "left";
					break;
				case GLFW_KEY_RIGHT:
					text += "right";
					break;
				case GLFW_KEY_ENTER:
					text += "enter";
					break;
				default:
					text += glfwGetKeyName(key.Info.key, 0);
					break;
			}
		}
		text += "###" + std::to_string(i);

		if(ImGui::Selectable(text.c_str(), i == selected)) {
			selected = i;
		}
	}

	const bool ret = changed;
	changed = false;
	return ret;
}

bool Input::TryGetAction(const Key val, Action& action) {
	if(!keyMap.count(val.Reg)) {
		return false;
	}
	action = keyMap.at(val.Reg);
	return true;
}

bool Input::TryGetAction(const int key, const int mods, Action& action) {
	Key val;
	val.Info.key = key;
	val.Info.mods = mods & modMask;

	return TryGetAction(val, action);
}

uint8_t Input::GetController(int id) {
	assert(id == 0 || id == 1);
	const int offset = id * 8;
	uint8_t val = 0;

	for(int i = 0; i < 8; ++i) {
		val |= keys[i + offset] << i;
	}

	return val;
}
