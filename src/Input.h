#pragma once
#include <cstdint>
#include <map>

#include <GLFW/glfw3.h>

#include "json.h"

union Key {
	struct {
		int key;
		int mods;

		
	} Info;
	uint64_t Reg;

	bool operator ==(Key other) {
		return Reg == other.Reg;
	}
};

struct InputItem {
	std::string Name;
	int Id;
	Key Default;
};

struct InputMapper {
	std::string name;
	std::map<int, Key> keyMap;
	std::vector<InputItem> items;

	int selected = -1;
	bool changed = false;

	InputMapper() { };
	
	InputMapper(const std::string& name, const std::vector<InputItem>& elements);

	bool ShowEditWindow();
	bool TryGetKey(int Id, Key& key);
	bool TryGetId(Key key, int& Id);
};

namespace Input {
	void OnKey(int key, int scancode, int action, int mods);

	void Load(Json& j);
	void Save(Json& j);

	bool ShowEditWindow();

	void SetMapper(InputMapper mapper);
	bool GetKey(int mappedId);
}
