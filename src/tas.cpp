#include "tas.h"

#include <cstdint>
#include <fstream>

enum Test : uint8_t {
	A = 1,
	B = 1 << 1,
	Start = 1 << 2,
	Select = 1 << 3,
	Up = 1 << 4,
	Down = 1 << 5,
	Left = 1 << 6,
	Right = 1 << 7
};

const int fm2KeyMap[] = {Right, Left, Down, Up, Select, Start, B, A};
const int bk2KeyMap[] = {Up, Down, Left, Right, Select, Start, B, A};

TasInputs Shit(std::string path, const int keyMap[], int start) {
	std::ifstream file(path);
	if(!file.is_open()) {
		throw std::logic_error("Tas file not found");
	}

	TasInputs inputs;

	std::string line;
	while(file.good()) {
		std::getline(file, line);

		if(line[0] == '|' && line[1] != '1') {
			int pos = start;
			uint8_t val = 0;

			if(line[pos] == '|') {
				inputs.Controller1.push_back(0);
			} else {
				for(int i = 0; i < 8; i++) {
					if(line[pos] != '.') {
						val |= keyMap[i];
					}
					pos++;
				}

				inputs.Controller1.push_back(val);
			}

			pos++;

			if(line[pos] == '|') {
				inputs.Controller2.push_back(0);
			} else {
				val = 0;
				
				for(int i = 0; i < 8; i++) {
					if(line[pos] != '.') {
						val |= keyMap[i];
					}
					pos++;
				}
				inputs.Controller2.push_back(val);
			}
		}
	}

	return inputs;
}

TasInputs TasInputs::LoadFM2(std::string path) {
	return Shit(path, fm2KeyMap, 3);
}

TasInputs TasInputs::LoadBK2(std::string path) {
	return Shit(path, bk2KeyMap, 4);
}
