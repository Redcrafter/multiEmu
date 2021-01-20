#include "Input.h"

#include <set>
#include <string>

#include <imgui.h>

InputMapper::InputMapper(const std::string& name, const std::vector<InputItem>& elements) {
	this->name = name;
	items = elements;

	for(auto item : items) {
		if(item.Default.Reg != 0) {
			keyMap[item.Id] = item.Default;
		}
	}
}

bool InputMapper::ShowEditWindow() {
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

		if(ImGui::Selectable(text.c_str(), i == selected)) {
			selected = i;
		}
	}

	const bool ret = changed;
	changed = false;
	return ret;
}

bool InputMapper::TryGetKey(int Id, Key& key) {
	if(keyMap.count(Id)) {
		key = keyMap[Id];
		return true;
	}
	return false;
}

bool InputMapper::TryGetId(Key key, int& Id) {
	for (auto &item : keyMap) {
		if(item.second == key) {
			Id = item.first;
			return true;
		}
	}
	return false;
}

static std::set<uint64_t> keys;
static InputMapper currentMapper;
static std::map<std::string, std::map<std::string, uint64_t>> knownMappers;

void Input::OnKey(int key, int scancode, int action, int mods) {
	if(key >= GLFW_KEY_LEFT_SHIFT) {
		return;
	}

	Key k;
	k.Info.key = key;
	k.Info.mods = mods;

	// TODO: somehow get keys from other viewports
	if(currentMapper.selected != -1) {
		currentMapper.changed = true;

		if(key == GLFW_KEY_BACKSPACE) {
			currentMapper.keyMap.erase(currentMapper.selected);
		} else {
			currentMapper.keyMap[currentMapper.selected] = k;
		}

		currentMapper.selected = -1;
	} else {
		if(action == GLFW_PRESS) {
			keys.insert(k.Reg);
		} else if(action == GLFW_RELEASE) {
			keys.erase(k.Reg);
		}
	}
}

void Input::Load(Json& j) {
	j["keymap"].tryGet(knownMappers);
}

void Input::Save(Json& j) {
	j["keymap"] = knownMappers;
}

bool Input::ShowEditWindow() {
	return currentMapper.ShowEditWindow();
}

void Input::SetMapper(const InputMapper& mapper) {
	currentMapper = mapper;

	auto& known = knownMappers[mapper.name];
	for (auto &&i : currentMapper.items) {
		if(known.count(i.Name)) {
			currentMapper.keyMap[i.Id].Reg = known[i.Name];
		}
	}

	known.clear();
	for(auto&& i : currentMapper.items) {
		known[i.Name] = currentMapper.keyMap[i.Id].Reg;
	}
}

bool Input::GetKey(int mappedId) {
	Key mapped;
	if(currentMapper.TryGetKey(mappedId, mapped)) {
		return keys.count(mapped.Reg);
	}
	return false;
}
