#include "settings.h"

#include <fstream>

#include "Input.h"
#include "fs.h"
#include "logger.h"

namespace Settings {

void Load() {
	Json j;

	if(fs::exists("./settings.json")) {
		try {
			std::ifstream file("./settings.json");
			if(!file.good()) {
				return;
			}
			file >> j;
		} catch(std::exception& e) {
			logger.LogScreen("Failed to load settings %s", e.what());
		}

		j["enableVsync"].tryGet(EnableVsync);
		j["autoHideMenu"].tryGet(AutoHideMenu);
		j["windowScale"].tryGet(windowScale);

		std::vector<std::string> files;
		j["recent"].tryGet(files);
		RecentFiles = std::deque<std::string>(files.begin(), files.end());

		Input::Load(j);
	}
}

void Save() {
	Json j = {
		{ "enableVsync", EnableVsync },
		{ "autoHideMenu", AutoHideMenu },
		{ "windowScale", windowScale },
		{ "recent", RecentFiles },
	};
	Input::Save(j);

	std::ofstream file("./settings.json");
	if(file.good()) file << j;
}

void AddRecent(std::string path) {
	for(size_t i = 0; i < RecentFiles.size(); ++i) {
		if(RecentFiles[i] == path) {
			RecentFiles.erase(RecentFiles.begin() + i);
			break;
		}
	}

	if(RecentFiles.size() >= 10) {
		RecentFiles.pop_back();
	}

	RecentFiles.push_front(std::move(path));

	Save();
}

} // namespace Settings
