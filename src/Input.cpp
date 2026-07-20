#include "Input.h"

#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include <imgui.h>

namespace Input {

static std::vector<Key> keyDown;
static std::vector<Key> keyHold;
static std::vector<Key> keyUp;

static std::vector<const char*> mapperNames;
static std::vector<Mapper*> mappers;

bool find(const std::vector<Key>& vec, Key key) {
	for(auto& item : vec) {
		if(item == key) return true;
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
		if(key.Info.key == SDL_SCANCODE_BACKSPACE) key.Reg = 0;
		key.Info.mods &= SDL_KMOD_CTRL | SDL_KMOD_SHIFT | SDL_KMOD_ALT;
		keyMap[items[selected].Id] = key;
		selected = -1;
	}

	for(size_t i = 0; i < items.size(); ++i) {
		auto& item = items[i];
		ImGui::Text("%s: ", item.Name.c_str());
		ImGui::SameLine(200);

		std::string text;
		if(keyMap.count(item.Id)) {
			Key key = keyMap[item.Id];

			if(key.Info.mods & SDL_KMOD_CTRL)
				text += "ctrl + ";
			if(key.Info.mods & SDL_KMOD_SHIFT)
				text += "shift + ";
			if(key.Info.mods & SDL_KMOD_ALT)
				text += "alt + ";

			if(key.Info.key == SDL_SCANCODE_UNKNOWN) {
				text += "none";
			} else {
				auto name = SDL_GetScancodeName(key.Info.key);
				if(name) {
					text += name;
				} else {
					text += "???";
				}
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
	return find(keyHold, keyMap[id]);
}

bool Mapper::GetKeyDown(int id) {
	assert(keyMap.count(id));
	return find(keyDown, keyMap[id]);
}

bool Mapper::GetKeyUp(int id) {
	assert(keyMap.count(id));
	return find(keyUp, keyMap[id]);
}

void Mapper::HandleKeyDown(const SDL_KeyboardEvent& event) {
	if(event.repeat) return;

	Key key { event.scancode, event.mod & (SDL_KMOD_CTRL | SDL_KMOD_SHIFT | SDL_KMOD_ALT) };
	keyDown.push_back(key);
	keyHold.push_back(key);
}
void Mapper::HandleKeyUp(const SDL_KeyboardEvent& event) {
	Key key { event.scancode, event.mod & (SDL_KMOD_CTRL | SDL_KMOD_SHIFT | SDL_KMOD_ALT) };
	keyUp.push_back(key);
	keyHold.erase(std::remove_if(keyHold.begin(), keyHold.end(), [key](const Key& k) { return k.Info.key == key.Info.key; }), keyHold.end());
}

void Mapper::Load(const nlohmann::json& j) {
	std::map<std::string, std::map<std::string, int>> temp = j["keymap"];

	for(size_t i = 0; i < mappers.size(); i++) {
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
	for(size_t i = 0; i < mappers.size(); i++) {
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
