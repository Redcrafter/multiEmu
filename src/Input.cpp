#include "Input.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <imgui.h>

namespace Input {

static std::vector<uint64_t> keyDown;
static std::vector<uint64_t> keyHold;
static std::vector<uint64_t> keyUp;

template<typename T>
bool find(const std::vector<T>&vec, T val) {
	for(auto& item: vec) {
		if(item == val) return true;
	}
	return false;
}

Mapper::Mapper(const char* name, const std::vector<InputItem>& elements) {
	items = elements;

	for(auto& [Name, Id, Default] : items) {
		keyMap[Id] = Default;
	}

	mapperNames.push_back(name);
	mappers.push_back(this);
}

void Mapper::ShowEditWindow() {
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
				case 0:
					text += "none";
					break;
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
				case GLFW_KEY_DELETE:
					text += "del";
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

bool Mapper::GetKey(int id) {
	assert(keyMap.count(id));
	return find(keyHold, keyMap[id].Reg);
}

bool Mapper::GetKeyDown(int id) {
	assert(keyMap.count(id));
	return find(keyDown, keyMap[id].Reg);
}

bool Mapper::GetKeyUp(int id) {
	assert(keyMap.count(id));
	return find(keyUp, keyMap[id].Reg);
}

void Mapper::OnKey(int key, int scancode, int action, int mods) {
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

void Mapper::Load(const nlohmann::json& j) {
	std::map<std::string, std::map<std::string, int>> temp = j["keymap"];

	for (size_t i = 0; i < mappers.size(); i++) {
		auto mapper = mappers[i];
		auto& items = temp[mapperNames[i]];

		for(auto& [Name, Id, Default] : mapper->items) {
			if(items.count(Name)) {
				mapper->keyMap[Id] = items[Name];
			}
		}
	}
}

void Mapper::Save(nlohmann::json& j) {
	std::map<std::string, std::map<std::string, int>> temp;

	std::map<std::string, int> keys;
	for (size_t i = 0; i < mappers.size(); i++) {
		auto mapper = mappers[i];
		keys.clear();

		for(auto& j : mapper->keyMap) {
			keys[std::to_string(j.first)] = j.second.Reg;
		}
		temp[mapperNames[i]] = keys;
	}

	j["keymap"] = temp;
}

void Mapper::DrawStuff() {
	ImGui::BeginChild("input");

	static int currentItem = 0;
	ImGui::Combo("##inputCombo", &currentItem, mapperNames.data(), mapperNames.size());

	assert(currentItem >= 0 && currentItem < mappers.size());
	mappers[currentItem]->ShowEditWindow();

	ImGui::EndChild();
}

void Mapper::NewFrame() {
	keyDown.clear();
	keyUp.clear();
}

}
