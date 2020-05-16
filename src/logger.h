#pragma once
#include <exception>
#include <string>

#include <imgui.h>

using namespace std::string_literals;

struct Logger {
	bool Show;
	ImGuiTextBuffer Buf;
	ImGuiTextFilter Filter;
	ImVector<int> LineOffsets; // Index to lines offset. We maintain this with AddLog() calls, allowing us to have a random access on lines
	bool AutoScroll;           // Keep scrolling if already at the bottom

	Logger();
	void Clear();
	
	void Log(const char* fmt, ...) IM_FMTARGS(2);

	void Draw();
};

extern Logger logger;
