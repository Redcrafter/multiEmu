#include "settings.h"

#include <fstream>

#include "Input.h"
#include "fs.h"
#include "logger.h"

namespace Settings {

void Load() {
	if(!fs::exists("./settings.json"))
		return;

	try {
		std::ifstream file("./settings.json");
		if(!file.good()) {
			return;
		}
		auto j = nlohmann::json::parse(file);

		EnableVsync = j["enableVsync"];
		AutoHideMenu = j["autoHideMenu"];
		windowScale = j["windowScale"];
		RecentFiles = j["recent"];
        GameInWindow = j["gameInWindow"];

		Input::Mapper::Load(j);
	} catch(std::exception& e) {
		logger.LogScreen("Failed to load settings %s", e.what());
	}
}

void Save() {
	nlohmann::json j = {
		{ "enableVsync", EnableVsync },
		{ "autoHideMenu", AutoHideMenu },
		{ "windowScale", windowScale },
		{ "recent", RecentFiles },
        { "gameInWindow", GameInWindow },
	};
	Input::Mapper::Save(j);

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
