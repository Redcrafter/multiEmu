#pragma once
#include <string>

class TasEditor {
public:
	std::string Title;
private:
	bool open = false;

	int selectedRow = -1;
public:
	TasEditor(std::string Title);

	void Open() { open = true; }
	void DrawWindow(int currentFrame);
};

