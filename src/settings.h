#pragma once
#include <deque>
#include <string>

namespace Settings {

// bug: stops working when switching to fullscreen
inline bool EnableVsync = true;
inline bool AutoHideMenu = true;
inline int windowScale = 4;
inline std::deque<std::string> RecentFiles;
inline bool GameInWindow = false;

void Load();
void Save();
void AddRecent(std::string path);

} // namespace Settings
