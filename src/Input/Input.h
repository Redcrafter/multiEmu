#pragma once
#include <cstdint>
#include <map>
#include "../json.hpp"

enum class Action {
	Controller1A,
	Controller1B,
	Controller1Start,
	Controller1Select,
	Controller1Up,
	Controller1Down,
	Controller1Left,
	Controller1Right,

	Controller2A,
	Controller2B,
	Controller2Start,
	Controller2Select,
	Controller2Up,
	Controller2Down,
	Controller2Left,
	Controller2Right,
	// TODO: spacial controllers

	Speedup,		 // Toggle speedup x5
	Step,			 // Start step & advance frame
	ResumeRun,		 // Resume running normally

	Reset,			 // Reset nes

	SaveState,		 // Save to selected savestate
	LoadState,		 // Load from selected savestate
	SelectNextState, // Select next savestate
	SelectLastState, // Select previous savestate
};

union Key {
	struct {
		uint32_t key;
		uint32_t mods;
	} Info;
	uint64_t Reg;
};

class Input {
private:
	static int keys[16];
	static std::map<uint64_t, Action> keyMap;
	static std::map<Action, uint64_t> revKeyMap;
public:
	static void OnKey(int key, int scancode, int action, int mods);

	static void Load(nlohmann::json& j);
	static void Save(nlohmann::json& j);

	static void ShowEditWindow();

	static bool TryGetAction(Key val, Action& action);
	static bool TryGetAction(int key, int mods, Action& action);
	static uint8_t GetController(int id);
};
