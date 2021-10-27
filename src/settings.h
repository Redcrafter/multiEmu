#pragma once
#include <deque>
#include <string>

namespace Settings {

// bug: stops working when switching to fullscreen
inline bool EnableVsync = false;
inline bool AutoHideMenu = true;
inline int windowScale = 2;
inline std::deque<std::string> RecentFiles;

void Load();
void Save();
void AddRecent(std::string path);

} // namespace Settings
