#pragma once
#include <deque>
#include <string>

namespace Settings {

inline bool EnableVsync = true;
inline bool AutoHideMenu = true;
inline int windowScale = 2;
inline std::deque<std::string> RecentFiles;

void Load();
void Save();
void AddRecent(std::string path);

void Open();
void Draw();

} // namespace Settings
