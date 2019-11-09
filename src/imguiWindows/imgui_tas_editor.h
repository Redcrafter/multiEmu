#pragma once
#include <string>

class TasEditor {
public:
	std::string Title;
private:
	bool open = false;
public:
	TasEditor(std::string Title);

	void Open() { open = true; }
	void DrawWindow();
};

