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
	for(int i = 0; i < items.size(); ++i) {
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
InputMapper currentMapper;

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

			for(auto& item : currentMapper.items) {
				if(item.Id == currentMapper.selected) {
					k.Reg = 0;
					item.Default = k;
				}
			}
		} else {
			currentMapper.keyMap[currentMapper.selected] = k;
			
			for(auto& item : currentMapper.items) {
				if(item.Id == currentMapper.selected) {
					item.Default = k;
				}
			}
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
	/*if(j.contains("keymap") && j["keymap"].asObject()) {
		for(const auto& item : *j["keymap"].asObject()) {
			keyMap[item.second] = StringAction[item.first];
		}
	}

	for(auto map : keyMap) {
		revKeyMap[map.second] = map.first;
	}*/
}

void Input::Save(Json& j) {
	/*std::map<std::string, uint64_t> map;

	for(auto item : keyMap) {
		map[ActionString[(int)item.second]] = item.first;
	}

	j["keymap"] = map;*/
}

bool Input::ShowEditWindow() {
	return currentMapper.ShowEditWindow();
}

void Input::SetMapper(InputMapper mapper) {
	currentMapper = mapper;
}

bool Input::GetKey(int mappedId) {
	Key mapped;
	if(currentMapper.TryGetKey(mappedId, mapped)) {
		return keys.count(mapped.Reg);
	}
	return false;
}
