#include "Input.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <imgui.h>

namespace Input {

std::vector<uint64_t> keyDown;
std::vector<uint64_t> keyHold;
std::vector<uint64_t> keyUp;

template<typename T>
bool find(const std::vector<T>&vec, T val) {
	for(auto& item: vec) {
		if(item == val) return true;
	}
	return false;
}

InputMapper hotkeys {{
	{ "Speedup",		 0, { GLFW_KEY_Q,           0 } },
	{ "Step",			 1, { GLFW_KEY_F,           0 } },
	{ "ResumeRun",		 2, { GLFW_KEY_G,           0 } },
	{ "Reset",			 3, { GLFW_KEY_R,           0 } },
	{ "HardReset",		 4, { 0,                    0 } },
	{ "SaveState",		 5, { GLFW_KEY_K,           0 } },
	{ "LoadState",		 6, { GLFW_KEY_L,           0 } },
	{ "SelectNextState", 7, { GLFW_KEY_KP_ADD,      0 } },
	{ "SelectLastState", 8, { GLFW_KEY_KP_SUBTRACT, 0 } },
	{ "Maximise",		 9, { GLFW_KEY_F11,         0 } } 
}};
InputMapper Chip8 = {{
	{"0", 0,  { GLFW_KEY_1, 0 } },
	{"1", 1,  { GLFW_KEY_2, 0 } },
	{"2", 2,  { GLFW_KEY_3, 0 } },
	{"3", 3,  { GLFW_KEY_4, 0 } },
	{"4", 4,  { GLFW_KEY_Q, 0 } },
	{"5", 5,  { GLFW_KEY_W, 0 } },
	{"6", 6,  { GLFW_KEY_E, 0 } },
	{"7", 7,  { GLFW_KEY_R, 0 } },
	{"8", 8,  { GLFW_KEY_A, 0 } },
	{"9", 9,  { GLFW_KEY_S, 0 } },
	{"A", 10, { GLFW_KEY_D, 0 } },
	{"B", 11, { GLFW_KEY_F, 0 } },
	{"C", 12, { GLFW_KEY_Y, 0 } },
	{"D", 13, { GLFW_KEY_X, 0 } },
	{"E", 14, { GLFW_KEY_C, 0 } },
	{"F", 15, { GLFW_KEY_V, 0 } },
}};
InputMapper GB = {{
	{ "Right",  0, { GLFW_KEY_RIGHT, 0 } },
	{ "Left",   1, { GLFW_KEY_LEFT,  0 } },
	{ "Up",     2, { GLFW_KEY_UP,    0 } },
	{ "Down",   3, { GLFW_KEY_DOWN,  0 } },
	{ "A",      4, { GLFW_KEY_A,     0 } },
	{ "B",      5, { GLFW_KEY_B,     0 } },
	{ "Select", 6, { GLFW_KEY_ENTER, 0 } },
	{ "Start",  7, { GLFW_KEY_S,     0 } },
}};
InputMapper NES = {{
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
}};

InputMapper::InputMapper(const std::vector<InputItem>& elements) {
	items = elements;

	for(auto& [Name, Id, Default] : items) {
		keyMap[Id] = Default;
	}
}

void InputMapper::ShowEditWindow() {
	if(selected != -1 && !keyDown.empty()) {
		Key key = *keyDown.begin();
		if(key.Info.key == GLFW_KEY_BACKSPACE) key.Reg = 0;
		keyMap[items[selected].Id] = Key(key);
		selected = -1;
	}

	for(size_t i = 0; i < items.size(); ++i) {
		auto& item = items[i];
		ImGui::Text("%s: ", item.Name.c_str());
		ImGui::SameLine(200);

		std::string text;
		if(keyMap.count(item.Id)) {
			Key key = keyMap[item.Id];

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
				case GLFW_KEY_F1:
				case GLFW_KEY_F2:
				case GLFW_KEY_F3:
				case GLFW_KEY_F4:
				case GLFW_KEY_F5:
				case GLFW_KEY_F6:
				case GLFW_KEY_F7:
				case GLFW_KEY_F8:
				case GLFW_KEY_F9:
				case GLFW_KEY_F10:
				case GLFW_KEY_F11:
				case GLFW_KEY_F12:
					text += "F" + std::to_string(key.Info.key - GLFW_KEY_F1 + 1);
					break;
				default:
					auto name = glfwGetKeyName(key.Info.key, 0);
					if(name) {
						text += name;
					} else {
						text += "???";
					}
					break;
			}
		}
		text += "###" + std::to_string(i);

		if(ImGui::Selectable(text.c_str(), (int)i == selected)) {
			selected = i;
		}
	}
}

bool InputMapper::GetKey(int id) {
	assert(keyMap.count(id));
	return find(keyHold, keyMap[id].Reg);
}

bool InputMapper::GetKeyDown(int id) {
	assert(keyMap.count(id));
	return find(keyDown, keyMap[id].Reg);
}

bool InputMapper::GetKeyUp(int id) {
	assert(keyMap.count(id));
	return find(keyUp, keyMap[id].Reg);
}

void OnKey(int key, int scancode, int action, int mods) {
	if(key >= GLFW_KEY_LAST) {
		return;
	}

	Key k { key, mods };

	// TODO: somehow get keys from other viewports?
	if(action == GLFW_PRESS) {
		keyDown.push_back(k.Reg);
		keyHold.push_back(k.Reg);
	} else if(action == GLFW_RELEASE) {
		keyUp.push_back(k.Reg);
		keyHold.erase(std::remove(keyHold.begin(), keyHold.end(), k.Reg), keyHold.end());	
	}
}

void Load(Json& j) {
	std::map<std::string, std::map<std::string, int>> temp;
	j["keymap"].tryGet(temp);

	auto parseStuff = [](InputMapper& mapper, std::map<std::string, int>& i) {
		for(auto& [Name, Id, Default] : mapper.items) {
			if(i.count(Name)) {
				mapper.keyMap[Id] = i[Name];
			}
		}
	};

	parseStuff(hotkeys, temp["hotkeys"]);
	parseStuff(Chip8, temp["Chip-8"]);
	parseStuff(GB, temp["GB"]);
	parseStuff(NES, temp["Nes"]);
}

void Save(Json& j) {
	std::map<std::string, std::map<std::string, int>> temp;

	auto saveStuff = [](InputMapper& mapper) {
		std::map<std::string, int> keys;
		for(auto& j : mapper.keyMap) {
			keys[std::to_string(j.first)] = j.second.Reg;
		}
		return keys;
	};

	temp["hotkeys"] = saveStuff(hotkeys);
	temp["Chip-8"] = saveStuff(Chip8);
	temp["GB"] = saveStuff(GB);
	temp["Nes"] = saveStuff(NES);

	j["keymap"] = temp;
}

void DrawStuff() {
	ImGui::BeginChild("input");

	const char* names[] = {
		"Hotkeys", "Chip-8", "Gameboy", "NES"
	};

	static int currentItem = 0;
	ImGui::Combo("##inputCombo", &currentItem, names, std::size(names));

	InputMapper* asdf;
	switch(currentItem) {
		case 0: hotkeys.ShowEditWindow(); break;
		case 1: Chip8.ShowEditWindow(); break;
		case 2: GB.ShowEditWindow(); break;
		case 3: NES.ShowEditWindow(); break;
		default: assert(false); break;
	}

	ImGui::EndChild();
}

void NewFrame() {
	keyDown.clear();
	keyUp.clear();
}

}
