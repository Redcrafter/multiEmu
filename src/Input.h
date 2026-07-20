#pragma once
#include <cstdint>
#include <map>

#include <SDL3/SDL_events.h>

#include <nlohmann/json.hpp>

namespace Input {

union Key {
	struct {
		SDL_Scancode key;
		SDL_Keymod mods;
	} Info;
	uint64_t Reg;

	Key() = default;
	Key(uint64_t Reg) : Reg(Reg) {}
	Key(SDL_Scancode key, SDL_Keymod mods) : Info({ key, mods }) {}

	bool operator==(const Key& other) const {
		return Reg == other.Reg;
	}
};
static_assert(sizeof(Key) == 8);

struct InputItem {
	std::string Name;
	int Id;
	Key Default;
};

class Mapper {
  private:
	std::map<int, Key> keyMap;
	std::vector<InputItem> items;

	int selected = -1;

  public:
	Mapper(const char* name, const std::vector<InputItem>& elements);

	void ShowEditWindow();

	bool GetKey(int id);
	bool GetKeyDown(int id);
	bool GetKeyUp(int id);

	static void HandleKeyDown(const SDL_KeyboardEvent& event);
	static void HandleKeyUp(const SDL_KeyboardEvent& event);

	static void Load(const nlohmann::json& j);
	static void Save(nlohmann::json& j);

	static void DrawStuff();
	static void NewFrame();
};

}
