#pragma once
#include <vector>
#include <string>

struct TasInputs {
	std::vector<uint8_t> Controller1;
	std::vector<uint8_t> Controller2;

	static TasInputs LoadFM2(std::string path);
	static TasInputs LoadBK2(std::string path);
};
