#pragma once
#include <cstdint>
#include <map>

#include <GLFW/glfw3.h>

#include "json.h"

namespace Input {

union Key {
	struct {
		int key;
		int mods;
	} Info;
	uint64_t Reg;

	Key() = default;
	Key(uint64_t Reg) : Reg(Reg) {}
	Key(int key, int mods) : Info({ key, mods }) {}

	bool operator==(Key other) {
		return Reg == other.Reg;
	}
};
static_assert(sizeof(Key) == 8);

struct InputItem {
	std::string Name;
	int Id;
	Key Default;
};

struct InputMapper {
	std::map<int, Key> keyMap;
	std::vector<InputItem> items;

	int selected = -1;

	InputMapper() = default;
	InputMapper(const std::vector<InputItem>& elements);

	void ShowEditWindow();

	bool GetKey(int id);
	bool GetKeyDown(int id);
	bool GetKeyUp(int id);
};

void OnKey(int key, int scancode, int action, int mods);

void Load(Json& j);
void Save(Json& j);

void DrawStuff();
void NewFrame();

extern InputMapper hotkeys;
extern InputMapper Chip8;
extern InputMapper GB;
extern InputMapper NES;

}
