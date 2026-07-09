#pragma once
#include <cstdint>
#include <map>

#include <GLFW/glfw3.h>

#include <nlohmann/json.hpp>

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
	inline static std::vector<const char*> mapperNames;
	inline static std::vector<Mapper*> mappers;

	std::map<int, Key> keyMap;
	std::vector<InputItem> items;

	int selected = -1;

  public:
	Mapper(const char* name, const std::vector<InputItem>& elements);

	void ShowEditWindow();

	bool GetKey(int id);
	bool GetKeyDown(int id);
	bool GetKeyUp(int id);

	static void OnKey(int key, int scancode, int action, int mods);

	static void Load(const nlohmann::json& j);
	static void Save(nlohmann::json& j);

	static void DrawStuff();
	static void NewFrame();
};

}
