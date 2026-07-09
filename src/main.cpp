#include <algorithm>
#include <array>
#include <deque>
#include <thread>

#include <GLFW/glfw3.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "nativefiledialog/nfd.h"

#include "Input.h"

#include "imguiWindows/imgui_memory_editor.h"
// #include "imguiWindows/imgui_tas_editor.h"

#include "audio.h"
#include "fs.h"
#include "logger.h"
#include "settings.h"

#include "Emulation/CHIP-8/core.h"
#include "Emulation/GB/GameboyCore.h"
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

GLFWwindow* window;

std::unique_ptr<ICore> emulationCore;

int selectedSaveState = 0;
std::array<nlohmann::json, 10> saveStates;

MemoryEditor memEdit;

bool settingsWindow = false;
bool metricsWindow = false;

bool speedUp = false;
bool running = false;
bool step = false;

bool isFullscreen = false;
bool menuBarOpen = false;

static ImVec2 CalcWindowSize() {
	// ImVec2 size = (emulationCore ? emulationCore->GetSize() : ImVec2(292, 240)) * Settings::windowScale;
	auto size = ImVec2(292, 240) * Settings::windowScale;

	if(!Settings::AutoHideMenu) {
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

		Settings::AddRecent(path);

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
				std::ifstream file(sPath);
				saveStates[i] = nlohmann::json::parse(file);
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
				LoadCore<Chip8::Core>(file);
				open = false;
			}
			if(ImGui::Button("NES")) {
				LoadCore<Nes::Core>(file);
				open = false;
			}
			if(ImGui::Button("Gameboy Color")) {
				LoadCore<Gameboy::Core>(file);
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
		LoadCore<Chip8::Core>(path);
	} else if(ext == ".gb" || ext == ".gbc" || ext == ".gbs") {
		LoadCore<Gameboy::Core>(path);
	} else {
		emulatorPicker.Open(path);
	}
}

static void SaveState(int number) {
	if(emulationCore == nullptr) {
		return;
	}
	auto& state = saveStates[number];
	state.clear();
	emulationCore->SaveState(state);

	try {
		fs::create_directories("./saves/" + emulationCore->GetName() + "/");
		auto path = "./saves/" + emulationCore->GetName() + "/" + emulationCore->GetRomHash().ToString() + "-" + std::to_string(number) + ".sav";
		if(fs::exists(path)) {
			fs::copy(path, path + ".old", fs::copy_options::overwrite_existing);
		}
		auto str = state.dump();
		std::ofstream out(path);
		out << str;

		logger.LogScreen("Saved state %i", number);
	} catch(std::exception& e) {
		logger.LogScreen("Error saving: %s", e.what());
	}
	saveStates[number] = std::move(state);
}

static void LoadState(int number) {
	if(emulationCore == nullptr) {
		return;
	}

	const auto& state = saveStates[number];
	if(state != nullptr) {
		emulationCore->LoadState(state);
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

static Input::Mapper hotkeys ("hotkeys", {
	{ "Speedup",		 0, { GLFW_KEY_Q,           0 } },
	{ "Step",			 1, { GLFW_KEY_F,           0 } },
	{ "ResumeRun",		 2, { GLFW_KEY_G,           0 } },
	{ "Reset",			 3, { GLFW_KEY_R,           0 } },
	{ "HardReset",		 4, { 0,                    0 } },
	{ "SaveState",		 5, { GLFW_KEY_K,           0 } },
	{ "LoadState",		 6, { GLFW_KEY_L,           0 } },
	{ "SelectNextState", 7, { GLFW_KEY_KP_ADD,      0 } },
	{ "SelectLastState", 8, { GLFW_KEY_KP_SUBTRACT, 0 } },
	{ "Maximise",		 9, { GLFW_KEY_F11,         0 } } 
});

static void onKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
	Input::Mapper::OnKey(key, scancode, action, mods);
	if(action == GLFW_PRESS && key == GLFW_KEY_F12) {
		metricsWindow = !metricsWindow;
	}
}

static void handleGuiInput() {
	if(hotkeys.GetKeyDown((int)Action::Speedup)) speedUp = !speedUp;

	if(hotkeys.GetKeyDown((int)Action::Step)) {
		step = true;
		running = false;
	}

	if(hotkeys.GetKeyDown((int)Action::ResumeRun)) running = true;
	if(hotkeys.GetKeyDown((int)Action::Reset) && emulationCore) emulationCore->Reset();
	if(hotkeys.GetKeyDown((int)Action::HardReset) && emulationCore) emulationCore->HardReset();
	if(hotkeys.GetKeyDown((int)Action::SaveState)) SaveState(selectedSaveState);
	if(hotkeys.GetKeyDown((int)Action::LoadState)) LoadState(selectedSaveState);

	if(hotkeys.GetKeyDown((int)Action::SelectNextState)) {
		selectedSaveState = (selectedSaveState + 1) % 10;
		logger.Log("Selected state %i\n", selectedSaveState);
	}

	if(hotkeys.GetKeyDown((int)Action::SelectLastState)) {
		selectedSaveState -= 1;
		if(selectedSaveState < 0) {
			selectedSaveState += 10;
		}
		logger.Log("Selected state %i\n", selectedSaveState);
	}

	if(hotkeys.GetKeyDown((int)Action::Maximise)) {
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
				if(ImGui::Checkbox("Vsync", &Settings::EnableVsync)) {
					glfwSwapInterval(Settings::EnableVsync);
					Settings::Save();
				}

				if(ImGui::Checkbox("Autohide Menubar", &Settings::AutoHideMenu)) {
					Settings::Save();
				}

				int val = Settings::windowScale - 1;
				static const char* drawModeNames[] = { "x1", "x2", "x3", "x4" };
				if(ImGui::Combo("DrawMode", &val, drawModeNames, 4)) {
					Settings::windowScale = val + 1;
					auto s = CalcWindowSize();
					glfwSetWindowSize(window, s.x, s.y);
					Settings::Save();
				}
				ImGui::EndTabItem();
			}

			if(ImGui::BeginTabItem("Input")) {
				Input::Mapper::DrawStuff();
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
	}
	ImGui::End();
}

static void drawMemoryEditor() {
	static int selectedDomain = 0;

	if(!emulationCore || !memEdit.Open) return;
	const auto& domains = emulationCore->GetMemoryDomains();
	if(selectedDomain > domains.size()) selectedDomain = 0;

	memEdit.ReadFn = [](const ImU8* mem, size_t off, void* user_data) { 
		return (ImU8)emulationCore->ReadMemory(selectedDomain, off);
	};
	memEdit.WriteFn = [](ImU8* mem, size_t off, ImU8 d, void* user_data) {
		emulationCore->WriteMemory(selectedDomain, off, d);
	};

	auto mem_size = domains[selectedDomain].Size;

	MemoryEditor::Sizes s;
	memEdit.CalcSizes(s, mem_size, 0);
	ImGui::SetNextWindowSize(ImVec2(s.WindowWidth, s.WindowWidth * 0.60f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(s.WindowWidth, FLT_MAX));

	if(ImGui::Begin("Memory Editor", &memEdit.Open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar)) {
		if(ImGui::BeginMenuBar()) {
			if(ImGui::BeginMenu("Memory Domain")) {
				for(auto& domain : domains) {
					if(ImGui::MenuItem(domain.Name.c_str())) {
						selectedDomain = domain.Id;
					}
				}
				ImGui::EndMenu();
			}

			if(ImGui::MenuItem("Export")) {
				std::string path;
				NFD::SaveDialog({}, "./", path, (GLFWwindow*)ImGui::GetMainViewport()->PlatformHandle);

				std::ofstream file { path, std::ios::binary };
				for(size_t i = 0; i < mem_size; i++) {
					file.put(memEdit.ReadFn(nullptr, i, memEdit.UserData));
				}
			}

			ImGui::EndMenuBar();
		}

		memEdit.DrawContents(nullptr, mem_size, 0);
		if(memEdit.ContentsWidthChanged) {
			memEdit.CalcSizes(s, mem_size, 0);
			ImGui::SetWindowSize(ImVec2(s.WindowWidth, ImGui::GetWindowSize().y));
		}
	}
	ImGui::End();
}

static void drawGui() {
	if((menuBarOpen || glfwGetWindowAttrib(window, GLFW_HOVERED) || !Settings::AutoHideMenu) && ImGui::BeginMainMenuBar()) {
		menuBarOpen = false;
		const auto enabled = emulationCore != nullptr;

		if(ImGui::BeginMenu("File")) {
			menuBarOpen = true;

			if(ImGui::MenuItem("Open ROM", "CTRL+O")) {
				std::string outPath;
				const auto res = NFD::OpenDialog({ 
					{ "Rom Files", { "nes", "nsf", "ch8" } },
					{ "NES", { "nes", "nsf" } },
					{ "CHIP-8", { "ch8" } },
					{ "Gameboy", { "gb", "gbc", "gbs" } }
					}, nullptr, outPath, window);

				if(res == NFD::Result::Okay) {
					OpenFile(outPath);
				}
			}
			if(ImGui::BeginMenu("Recent ROMs")) {
				if(!Settings::RecentFiles.empty()) {
					for(const auto& str : Settings::RecentFiles) {
						if(ImGui::MenuItem(str.c_str())) {
							OpenFile(str);
						}
					}
				} else {
					ImGui::MenuItem("None", nullptr, false, false);
				}

				ImGui::Separator();
				if(ImGui::MenuItem("Clear", nullptr, false, !Settings::RecentFiles.empty())) {
					Settings::RecentFiles.clear();
					Settings::Save();
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
			menuBarOpen = true;

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
			menuBarOpen = true;

			if(ImGui::MenuItem("Settings")) {
				settingsWindow = true;
			}

			if(ImGui::MenuItem("Hex Editor", nullptr, false, enabled)) {
				memEdit.Open = true;
			}
			if(ImGui::MenuItem("Log")) {
				logger.Show = true;
			}

			ImGui::EndMenu();
		}

		if(emulationCore) {
			emulationCore->DrawMenuBar(menuBarOpen);
		}

		ImGui::EndMainMenuBar();
	}

	auto dockspaceId = ImGui::DockSpaceOverViewport();
	ImGui::SetNextWindowDockID(dockspaceId, ImGuiCond_FirstUseEver);

	if(emulationCore) {
		emulationCore->Draw();
	}
	logger.DrawScreen();

	drawMemoryEditor();
	logger.DrawWindow();

	if(metricsWindow) {
		ImGui::ShowMetricsWindow(&metricsWindow);
	}
	drawSettings();
	emulatorPicker.Draw();
}

int main(int argc, char* argv[]) {
	Settings::Load();
	Audio::Init();

	#pragma region glfw Init
	glfwSetErrorCallback(onGlfwError);
	if(!glfwInit()) {
		logger.Log("Failed to initialize GLFW\n");
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);			   // 4x antialiasing
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // We want OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3); //
	#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
	#endif
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL

	// TODO: glfwWindowHint(GLFW_DECORATED, false);

	auto s = ImVec2(256, 256) * Settings::windowScale;
	// Open a window and create its OpenGL context
	window = glfwCreateWindow(s.x, s.y, "multiEmu", nullptr, nullptr);
	if(window == nullptr) {
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(Settings::EnableVsync);

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
		if(!Settings::EnableVsync) {
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

		if((running || step) && emulationCore != nullptr) {
			if(speedUp) {
				for(size_t i = 1; i < 5; i++) {
					emulationCore->Update();
				}
			} else {
				emulationCore->Update();
			}
			Audio::Resample();
			step = false;
		}
		handleGuiInput();
		drawGui();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}

		Input::Mapper::NewFrame();

		glfwSwapBuffers(window);
		glfwWaitEventsTimeout(0.007);
	} while(glfwWindowShouldClose(window) == 0);
	#pragma endregion

	Settings::Save();
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
	return main(__argc, __argv);
}
#endif
