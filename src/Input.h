#pragma once
#include <cstdint>
#include <map>
#include <GLFW/glfw3.h>

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
	ChangePallet, 	 // Change selected pallet
	Step,			 // Start step & advance frame
	ResumeRun,		 // Resume running normally

	Reset,			 // Reset nes

	SaveState,		 // Save to selected savestate
	LoadState,		 // Load from selected savestate
	SelectNextState, // Select next savestate
	SelectLastState, // Select previous savestate
};

class Input {
private:
	static int keys[16];
	static std::map<int, Action> keyMap;
public:
	static void LoadKeyMap();
	static void OnKey(int key, int scancode, int action, int mods);

	static bool TryGetAction(int key, Action& action);
	static uint8_t GetController(int id);
};
