#pragma once
#include <chrono>
#include <exception>
#include <string>
#include <vector>

#include <imgui.h>

struct ScreenLogItem {
	std::string string;
	std::chrono::steady_clock::time_point creationTime;
};

struct Logger {
	bool Show = false;
	ImGuiTextBuffer Buf;
	ImGuiTextFilter Filter;
	ImVector<int> LineOffsets; // Index to lines offset. We maintain this with AddLog() calls, allowing us to have a random access on lines
	bool AutoScroll;           // Keep scrolling if already at the bottom

	std::vector<ScreenLogItem> logItems;

	Logger();
	void Clear();

	void LogScreen(const char* fmt, ...) IM_FMTARGS(2);
	void Log(const char* fmt, ...) IM_FMTARGS(2);

	void DrawWindow();
	void DrawScreen();
};

extern Logger logger;
