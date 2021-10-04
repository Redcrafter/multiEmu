#include <algorithm>
#include <array>
#include <deque>
#include <fstream>
#include <thread>

#include <GLFW/glfw3.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "nativefiledialog/nfd.h"

#include "Input.h"
#include "saver.h"

#include "imguiWindows/imgui_memory_editor.h"
// #include "imguiWindows/imgui_tas_editor.h"

#include "audio.h"
#include "logger.h"
#include "fs.h"
#include "json.h"

#include "Emulation/CHIP-8/chip8.h"
#include "Emulation/NES/NesCore.h"

enum class Action {
	Speedup,   // Toggle speedup x5
	Step,	   // Start step & advance frame
	ResumeRun, // Resume running normally

	Reset,
	HardReset,

	SaveState,		 // Save to selected savestate
	LoadState,		 // Load from selected savestate
	SelectNextState, // Select next savestate
	SelectLastState, // Select previous savestate

	Maximise
};

Input::InputMapper hotkeys { "hotkeys", {
	{ "Speedup",		 (int)Action::Speedup,		   { { GLFW_KEY_Q, 0 } } },
	{ "Step",			 (int)Action::Step,			   { { GLFW_KEY_F, 0 } } },
	{ "ResumeRun",		 (int)Action::ResumeRun,	   { { GLFW_KEY_G, 0 } } },
	{ "Reset",			 (int)Action::Reset,	 	   { { GLFW_KEY_R, 0 } } },
	{ "HardReset",		 (int)Action::HardReset,	   { { 0, 0 } } },
	{ "SaveState",		 (int)Action::SaveState,	   { { GLFW_KEY_K, 0 } } },
	{ "LoadState",		 (int)Action::LoadState,	   { { GLFW_KEY_L, 0 } } },
	{ "SelectNextState", (int)Action::SelectNextState, { { GLFW_KEY_KP_ADD, 0 } } },
	{ "SelectLastState", (int)Action::SelectLastState, { { GLFW_KEY_KP_SUBTRACT, 0 } } },
	{ "Maximise",		 (int)Action::Maximise, 	   { { GLFW_KEY_F11, 0 } } } 
} };

GLFWwindow* window;

std::unique_ptr<ICore> emulationCore;

int selectedSaveState = 0;
std::array<std::unique_ptr<saver>, 10> saveStates;

MemoryEditor memEdit { "Memory Editor" };

bool settingsWindow = false;
bool metricsWindow = false;

bool speedUp = false;
bool running = false;
bool step = false;

bool isFullscreen = false;

struct {
	bool EnableVsync = true;
	bool AutoHideMenu = true;
	bool UseDockingWindow = false;
	int windowScale = 2;

	std::deque<std::string> RecentFiles;

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
			j["useDockingWindow"].tryGet(UseDockingWindow);
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
			{ "useDockingWindow", UseDockingWindow }
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

	void ClearRecent() {
		RecentFiles.clear();
		Save();
	}
} settings;

static ImVec2 CalcWindowSize() {
	ImVec2 size;

	if(emulationCore) {
		auto texture = emulationCore->GetMainTexture();
		size = ImVec2(texture->GetWidth() * emulationCore->GetPixelRatio(), texture->GetHeight());
	} else {
		size = ImVec2(292, 240);
	}

	size *= settings.windowScale;
	if(!settings.AutoHideMenu) {
		auto w = ImGui::FindWindowByName("##MainMenuBar");
		size.y += w->MenuBarHeight();
	}
	return size;
}

template<typename T>
static void LoadCore(const std::string& path) {
	if(emulationCore == nullptr || typeid(*emulationCore) != typeid(T)) {
		emulationCore = std::make_unique<T>();
		auto s = CalcWindowSize();
		glfwSetWindowSize(window, s.x, s.y);
	}
	try {
		emulationCore->LoadRom(path);

		settings.AddRecent(path);

		memEdit.SetCore(emulationCore.get());

		running = true;
	} catch(std::exception& e) {
		logger.LogScreen("Failed to load ROM: %s", e.what());
	}

	// Load savestates
	std::string partial = "./saves/" + emulationCore->GetName() + "/" + emulationCore->GetRomHash().ToString() + "-";
	for(int i = 0; i < 10; ++i) {
		std::string sPath = partial + std::to_string(i) + ".sav";

		if(fs::exists(sPath)) {
			try {
				saveStates[i] = std::make_unique<saver>(sPath);
			} catch(std::exception& e) {
				logger.Log("Error loading state: %s\n%s\n", sPath.c_str(), e.what());
			}
		}
	}
}

struct {
	bool open = false;
	std::string file;

	void Open(const std::string& path) {
		open = true;
		file = path;
	}

	void Draw() {
		if(!open)
			return;

		if(ImGui::Begin("Picker", &open)) {
			ImGui::Text("Unknown file type. Choose an emulator:");

			if(ImGui::Button("Chip-8")) {
				LoadCore<Chip8Core>(file);
				open = false;
			}
			if(ImGui::Button("NES")) {
				LoadCore<Nes::Core>(file);
				open = false;
			}
		}
		ImGui::End();
	}
} emulatorPicker;

static void OpenFile(const std::string& path) {
	auto ext = fs::path(path).extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });

	if(ext == ".nes" || ext == ".nsf" || ext == ".fm2") {
		LoadCore<Nes::Core>(path);
	} else if(ext == ".ch8") {
		LoadCore<Chip8Core>(path);
	} else {
		emulatorPicker.Open(path);
	}
}

static void SaveState(int number) {
	if(emulationCore == nullptr) {
		return;
	}
	saveStates[number] = std::make_unique<saver>();
	auto& state = saveStates[number];
	emulationCore->SaveState(*state);

	try {
		fs::create_directories("./saves/" + emulationCore->GetName() + "/");
		auto path = "./saves/" + emulationCore->GetName() + "/" + emulationCore->GetRomHash().ToString() + "-" + std::to_string(number) + ".sav";
		if(fs::exists(path)) {
			fs::copy(path, path + ".old", fs::copy_options::overwrite_existing);
		}
		state->Save(path);

		logger.LogScreen("Saved state %i", number);
	} catch(std::exception& e) {
		logger.LogScreen("Error saving: %s", e.what());
	}
}

static void LoadState(int number) {
	if(emulationCore == nullptr) {
		return;
	}

	const auto& state = saveStates[number];
	if(state != nullptr) {
		state->readPos = 0;
		emulationCore->LoadState(*state);

		logger.LogScreen("Loaded state %i", number);
	} else {
		logger.LogScreen("Save state %i empty", number);
	}
}

static void HelpMarker(const char* desc) {
	// ImGui::TextDisabled("(?)");
	if(ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

static void onKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
	Input::OnKey(key, scancode, action, mods);

	if(action != GLFW_PRESS) {
		return;
	}

	if(key == GLFW_KEY_F12) {
		metricsWindow = !metricsWindow;
	}

	Input::Key k { { key, mods } };

	int mapped;
	if(!hotkeys.TryGetId(k, mapped)) {
		return;
	}

	switch((Action)mapped) {
		case Action::Speedup:
			speedUp = !speedUp;
			break;
		case Action::Step:
			step = true;
			running = false;
			break;
		case Action::ResumeRun:
			running = true;
			break;
		case Action::Reset:
			if(emulationCore) {
				emulationCore->Reset();
			}
			break;
		case Action::HardReset:
			if(emulationCore) {
				emulationCore->HardReset();
			}
			break;
		case Action::SaveState:
			SaveState(selectedSaveState);
			break;
		case Action::LoadState:
			LoadState(selectedSaveState);
			break;
		case Action::SelectNextState:
			selectedSaveState = (selectedSaveState + 1) % 10;
			logger.Log("Selected state %i\n", selectedSaveState);
			break;
		case Action::SelectLastState:
			selectedSaveState -= 1;
			if(selectedSaveState < 0) {
				selectedSaveState += 10;
			}
			logger.Log("Selected state %i\n", selectedSaveState);
			break;
		case Action::Maximise:
			if(!isFullscreen) {
				isFullscreen = true;

				const auto monitor = glfwGetPrimaryMonitor();
				const GLFWvidmode* mode = glfwGetVideoMode(monitor);
				glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
			} else {
				isFullscreen = false;

				auto s = CalcWindowSize();
				glfwSetWindowMonitor(window, nullptr, 50, 50, s.x, s.y, 60);
			}
			break;
	}
}

static void onResize(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

static void onDrop(GLFWwindow* window, int count, const char** paths) {
	if(!is_regular_file(fs::path(paths[0]))) {
		return;
	}

	OpenFile(paths[0]);
}

static void onGlfwError(int error, const char* description) {
	logger.Log("Glfw Error: %d: %s\n", error, description);
}

static void drawSettings() {
	if(!settingsWindow)
		return;

	if(ImGui::Begin("Settings", &settingsWindow)) {
		if(ImGui::BeginTabBar("tabBar")) {
			if(ImGui::BeginTabItem("General")) {
				if(ImGui::Checkbox("Vsync", &settings.EnableVsync)) {
					glfwSwapInterval(settings.EnableVsync);
					settings.Save();
				}
				HelpMarker("Toggle Vsync.\nReduces cpu usage on fast cpu's but can cause stuttering");

				if(ImGui::Checkbox("Autohide Menubar", &settings.AutoHideMenu)) {
					settings.Save();
				}

				if(ImGui::Checkbox("Use dockspace", &settings.UseDockingWindow)) {
					settings.Save();
				}

				int val = settings.windowScale - 1;
				static const char* drawModeNames[] = { "x1", "x2", "x3", "x4" };
				if(ImGui::Combo("DrawMode", &val, drawModeNames, 4)) {
					settings.windowScale = val + 1;
					auto s = CalcWindowSize();
					glfwSetWindowSize(window, s.x, s.y);
					settings.Save();
				}
				ImGui::EndTabItem();
			}

			if(ImGui::BeginTabItem("Hotkeys")) {
				ImGui::BeginChild("hotkeys");

				hotkeys.ShowEditWindow();

				ImGui::EndChild();
				ImGui::EndTabItem();
			}

			if(ImGui::BeginTabItem("Controllers")) {
				ImGui::BeginChild("inputs");
				if(Input::ShowEditWindow()) {
					settings.Save();
				}
				ImGui::EndChild();

				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
	ImGui::End();
}

static void drawGui() {
	// fix for menubar closing when menu is too big and creates new context
	static bool menuOpen = false;
	static bool lastDock = settings.UseDockingWindow;

	bool enabled = emulationCore != nullptr;

	bool showMenuBar = menuOpen || glfwGetWindowAttrib(window, GLFW_HOVERED);
	menuOpen = false;
	if((showMenuBar || !settings.AutoHideMenu) && ImGui::BeginMainMenuBar()) {
		if(ImGui::BeginMenu("File")) {
			menuOpen = true;

			if(ImGui::MenuItem("Open ROM", "CTRL+O")) {
				std::string outPath;
				const auto res = NFD::OpenDialog({ { "Rom Files", { "nes", "nsf", "ch8" } }, { "NES", { "nes", "nsf" } }, { "CHIP-8", { "ch8" } } }, nullptr, outPath, window);

				if(res == NFD::Result::Okay) {
					OpenFile(outPath);
				}
			}
			if(ImGui::BeginMenu("Recent ROMs")) {
				if(!settings.RecentFiles.empty()) {
					for(const auto& str : settings.RecentFiles) {
						if(ImGui::MenuItem(str.c_str())) {
							OpenFile(str);
						}
					}
				} else {
					ImGui::MenuItem("None", nullptr, false, false);
				}

				ImGui::Separator();
				if(ImGui::MenuItem("Clear", nullptr, false, !settings.RecentFiles.empty())) {
					settings.ClearRecent();
				}

				ImGui::EndMenu();
			}

			ImGui::Separator();

			if(ImGui::BeginMenu("Save State", enabled)) {
				for(int i = 0; i < 10; ++i) {
					std::string str = "Shift+" + std::to_string(i);

					if(ImGui::MenuItem(str.c_str(), str.c_str())) {
						SaveState(i);
					}
				}
				ImGui::EndMenu();
			}
			if(ImGui::BeginMenu("Load State", enabled)) {
				for(int i = 0; i < 10; ++i) {
					std::string str = std::to_string(i);

					if(ImGui::MenuItem(str.c_str(), ("Shift+" + str).c_str(), false, saveStates[i] != nullptr)) {
						LoadState(i);
					}
				}
				ImGui::EndMenu();
			}

			if(ImGui::BeginMenu("Save Slot", enabled)) {
				for(int i = 0; i < 10; ++i) {
					std::string str = "Select Slot " + std::to_string(i);

					bool selected = selectedSaveState == i;
					ImGui::Checkbox(str.c_str(), &selected);

					if(selected) {
						selectedSaveState = i;
					}
				}

				ImGui::EndMenu();
			}

			ImGui::Separator();
			if(ImGui::MenuItem("Exit", "Alt+F4")) {
				glfwSetWindowShouldClose(window, true);
			}

			ImGui::EndMenu();
		}

		if(ImGui::BeginMenu("Emulation")) {
			menuOpen = true;

			bool paused = !running;
			ImGui::Checkbox("Pause", &paused);
			running = !paused;

			ImGui::Separator();

			if(ImGui::MenuItem("Soft Reset", nullptr, false, enabled)) {
				if(emulationCore) {
					emulationCore->Reset();
				}
			}
			if(ImGui::MenuItem("Hard Reset", nullptr, false, enabled)) {
				if(emulationCore) {
					emulationCore->HardReset();
				}
			}

			ImGui::EndMenu();
		}

		if(ImGui::BeginMenu("Tools")) {
			menuOpen = true;

			if(ImGui::MenuItem("Settings")) {
				settingsWindow = true;
			}

			if(ImGui::MenuItem("Hex Editor", nullptr, false, enabled)) {
				memEdit.Open();
			}
			if(ImGui::MenuItem("Log")) {
				logger.Show = true;
			}

			ImGui::EndMenu();
		}

		if(emulationCore) {
			emulationCore->DrawMenuBar(menuOpen);
		}

		ImGui::EndMainMenuBar();
	}

	if(settings.UseDockingWindow) {
		auto dockspace_id = ImGui::DockSpaceOverViewport();

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0);

		if(!lastDock) {
			ImGui::SetNextWindowDockID(dockspace_id, ImGuiCond_Always);
		}

		ImGui::Begin("NES", nullptr, window_flags);
		ImGui::PopStyleVar(2);
	} else {
		ImGuiWindowFlags window_flags =
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoBackground |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoNavFocus |
			ImGuiWindowFlags_NoDocking;

		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("NES", nullptr, window_flags);
		ImGui::PopStyleVar(3);
	}
	lastDock = settings.UseDockingWindow;

	auto windowSize = ImGui::GetWindowSize();
	if(emulationCore) {
		auto texture = emulationCore->GetMainTexture();

		auto width = texture->GetWidth();
		auto height = texture->GetHeight();

		auto ratio = height / (width * emulationCore->GetPixelRatio());

		auto size = ImGui::GetWindowSize();
		if(size.x * ratio < size.y) {
			size.y = size.x * ratio;
		} else {
			size.x = size.y * (1 / ratio);
		}

		ImGui::SetCursorPos((windowSize - size) * 0.5);

		texture->BufferImage();
		ImGui::Image(reinterpret_cast<void*>(texture->GetTextureId()), size);
	}
	logger.DrawScreen();
	ImGui::End();

	if(emulationCore) {
		emulationCore->DrawWindows();
	}
	memEdit.DrawWindow();
	logger.DrawWindow();

	if(metricsWindow) {
		ImGui::ShowMetricsWindow(&metricsWindow);
	}
	drawSettings();
	emulatorPicker.Draw();
}

int main() {
	settings.Load();

	Audio::Init();

	#pragma region glfw Init
	glfwSetErrorCallback(onGlfwError);
	if(!glfwInit()) {
		logger.Log("Failed to initialize GLFW\n");
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);                               // 4x antialiasing
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);                 // We want OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);                 //
	#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // To make MacOS happy; should not be needed
	#endif
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL

	// TODO: glfwWindowHint(GLFW_DECORATED, false);

	auto s = ImVec2(256, 256) * settings.windowScale;
	// Open a window and create its OpenGL context
	window = glfwCreateWindow(s.x, s.y, "Multi Emulator", nullptr, nullptr);
	if(window == nullptr) {
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(settings.EnableVsync);

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	glfwSetKeyCallback(window, onKey); // TODO: support controllers
	glfwSetWindowSizeCallback(window, onResize);
	glfwSetDropCallback(window, onDrop);
	#pragma endregion

	#pragma region ImGui Init
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();
	#pragma endregion

	onResize(window, s.x, s.y);

	auto lastTime = glfwGetTime();

	#pragma region render loop
	do {
		#pragma region Timing
		auto time = glfwGetTime();
		auto dt = time - lastTime;

		// if vsync is enabled glfw will wait in glfwPollEvents
		if(!settings.EnableVsync) {
			// otherwise we have to manually time the render loop
			if(dt >= 2 / 60.0) {
				// dropped frame
				// too much drift so we reset the timer
				lastTime = time;
				logger.Log("Dropped frame\n");
			} else if(dt >= 1 / 60.0) {
				// add time for 1 frame to stabilize framerate
				lastTime += 1 / 60.0;
			} else {
				// sleep for rest of frame
				std::this_thread::sleep_for(std::chrono::microseconds((int)((1 / 60.0 - dt) * std::micro::den)));
				continue;
			}
		} else {
			if(dt >= 2 / 60.0) {
				logger.Log("Dropped frame\n");
			}
			lastTime = time;
		}
		#pragma endregion

		glClear(GL_COLOR_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		if(running && emulationCore != nullptr) {
			emulationCore->Update();
			Audio::Resample();
		}
		drawGui();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	} while(glfwWindowShouldClose(window) == 0);
	#pragma endregion

	settings.Save();
	Audio::Dispose();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

#ifdef _WIN32
#include <windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	main();
}
#endif
