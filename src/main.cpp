#include <algorithm>
#include <array>
#include <deque>
#include <thread>

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <backends/imgui_impl_opengl3.h>
#include <backends/imgui_impl_sdl3.h>
#include <imgui.h>
#include <imgui_internal.h>

#include "imgui_memory_editor.h"
// #include "imguiWindows/imgui_tas_editor.h"

#include "Input.h"
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

SDL_Window* g_window;
SDL_GLContext gl_context;

std::unique_ptr<ICore> emulationCore;

std::chrono::steady_clock::time_point lastMouseMove;

int selectedSaveState = 0;
std::array<nlohmann::json, 10> saveStates;

MemoryEditor memEdit;
int memEdit_domain;

bool settingsWindow = false;
bool metricsWindow = false;

bool speedUp = false;
bool running = false;
bool step = false;

bool isFullscreen = false;
bool menuBarOpen = false;

bool shouldQuit = false;

static ImVec2 CalcWindowSize() {
	ImVec2 size = ImVec2(292, 240);
	if(emulationCore) {
		size = emulationCore->GetSize();
	}
	size *= Settings::windowScale;
	if(!Settings::AutoHideMenu) {
		size.y += ImGui::GetFrameHeight();
	}
	return size;
}

template<typename T>
static void LoadCore(const std::string& path) {
	if(emulationCore == nullptr || typeid(*emulationCore) != typeid(T)) {
		emulationCore = std::make_unique<T>();
		auto s = CalcWindowSize();
		SDL_SetWindowSize(g_window, s.x, s.y);
	}
	try {
		emulationCore->LoadRom(path);

		Settings::AddRecent(path);

		running = true;
	} catch(std::exception& e) {
		emulationCore = nullptr;
		logger.LogScreen("Failed to load ROM: %s", e.what());
		return;
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

// clang-format off
static Input::Mapper hotkeys("hotkeys", {
	{ "Speedup",		 0, { SDL_SCANCODE_Q,        0 } },
	{ "Step",			 1, { SDL_SCANCODE_F,        0 } },
	{ "ResumeRun",		 2, { SDL_SCANCODE_G,        0 } },
	{ "Reset",			 3, { SDL_SCANCODE_UNKNOWN,  0 } },
	{ "HardReset",		 4, { SDL_SCANCODE_R,        0 } },
	{ "SaveState",		 5, { SDL_SCANCODE_K,        0 } },
	{ "LoadState",		 6, { SDL_SCANCODE_L,        0 } },
	{ "SelectNextState", 7, { SDL_SCANCODE_KP_PLUS,  0 } },
	{ "SelectLastState", 8, { SDL_SCANCODE_KP_MINUS, 0 } },
	{ "Maximise",		 9, { SDL_SCANCODE_F11,      0 } },
});
// clang-format on

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
		isFullscreen = !isFullscreen;
		SDL_SetWindowFullscreen(g_window, isFullscreen);
	}
}

static void drawSettings() {
	if(!settingsWindow)
		return;

	if(ImGui::Begin("Settings", &settingsWindow)) {
		if(ImGui::BeginTabBar("tabBar")) {
			if(ImGui::BeginTabItem("General")) {
				if(ImGui::Checkbox("Vsync", &Settings::EnableVsync)) {
					SDL_GL_SetSwapInterval(Settings::EnableVsync);
					Settings::Save();
				}

				if(ImGui::Checkbox("Autohide Menubar", &Settings::AutoHideMenu)) {
					Settings::Save();
				}

				if(ImGui::Checkbox("Render game in own ImGui window", &Settings::GameInWindow)) {
					Settings::Save();
				}

				if(ImGui::SliderInt("Screen Scale", &Settings::windowScale, 1, 16)) {
					auto s = CalcWindowSize();
					SDL_SetWindowSize(g_window, s.x, s.y);
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
	if(!emulationCore || !memEdit.Open) return;
	const auto& domains = emulationCore->GetMemoryDomains();
	if(memEdit_domain > domains.size()) memEdit_domain = 0;

	memEdit.ReadFn = [](const ImU8* mem, size_t off, void* user_data) {
		return (ImU8)emulationCore->ReadMemory(memEdit_domain, off);
	};
	memEdit.WriteFn = [](ImU8* mem, size_t off, ImU8 d, void* user_data) {
		emulationCore->WriteMemory(memEdit_domain, off, d);
	};

	auto mem_size = domains[memEdit_domain].Size;

	MemoryEditor::Sizes s;
	memEdit.CalcSizes(s, mem_size, 0);
	ImGui::SetNextWindowSize(ImVec2(s.WindowWidth, s.WindowWidth * 0.60f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(s.WindowWidth, FLT_MAX));

	if(ImGui::Begin("Memory Editor", &memEdit.Open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar)) {
		if(ImGui::BeginMenuBar()) {
			if(ImGui::BeginMenu("Memory Domain")) {
				for(auto& domain : domains) {
					if(ImGui::MenuItem(domain.Name.c_str())) {
						memEdit_domain = domain.Id;
					}
				}
				ImGui::EndMenu();
			}

			if(ImGui::MenuItem("Export")) {
				SDL_ShowSaveFileDialog([](void* userdata, const char* const* filelist, int filter) {
					if(filelist == nullptr) {
						logger.LogScreen("Error saving: %s", SDL_GetError());
						return;
					}
					if(filelist[0] == nullptr) return;

					const auto& domains = emulationCore->GetMemoryDomains();
					auto mem_size = domains[memEdit_domain].Size;

					std::ofstream file { filelist[0], std::ios::binary };
					for(size_t i = 0; i < mem_size; i++) {
						file.put(emulationCore->ReadMemory(memEdit_domain, i));
					}
				},
					nullptr, g_window, nullptr, 0, "./");
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
	if((menuBarOpen || !Settings::AutoHideMenu || (((SDL_GetWindowFlags(g_window) & SDL_WINDOW_MOUSE_FOCUS) != 0) && (std::chrono::steady_clock::now() - lastMouseMove) < std::chrono::seconds(2))) && ImGui::BeginMainMenuBar()) {
		menuBarOpen = false;
		const auto enabled = emulationCore != nullptr;

		if(ImGui::BeginMenu("File")) {
			menuBarOpen = true;

			if(ImGui::MenuItem("Open ROM", "CTRL+O")) {
				SDL_DialogFileFilter filters[] = {
					{ "Rom Files", "nes;nsf;ch8;gb;gbc;gbs" },
					{ "NES", "nes;nsf" },
					{ "CHIP-8", "ch8" },
					{ "Gameboy", "gb;gbc;gbs" },
					{ "All files", "*" }
				};
				SDL_ShowOpenFileDialog([](void* userdata, const char* const* filelist, int filter) {
					if(filelist == nullptr) {
						logger.LogScreen("Error saving: %s", SDL_GetError());
						return;
					}
					if(filelist[0] == nullptr) return;

					OpenFile(filelist[0]);
				},
					nullptr, g_window, filters, sizeof(filters) / sizeof(filters[0]), "./", false);
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
				shouldQuit = true;
			}

			ImGui::EndMenu();
		}

		if(ImGui::BeginMenu("Emulation")) {
			menuBarOpen = true;

			bool paused = !running;
			ImGui::Checkbox("Pause", &paused);
			running = !paused;
			ImGui::Checkbox("Fast Forward", &speedUp);

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

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
	memEdit.Open = false;

	// Benchmark();
	// exit(1);
	// return SDL_APP_SUCCESS;

	#pragma region SDL Setup

	if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) {
		printf("Error: SDL_Init(): %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	Settings::Load();
	Audio::Init();

	#if defined(__APPLE__)
	// GL 3.2 Core + generally GLSL 150
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG); // Always required on Mac
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
	#else
	// GL 3.0 + generally GLSL 130
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	#endif

	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	float main_scale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
	SDL_WindowFlags window_flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
	g_window = SDL_CreateWindow("Dear ImGui SDL3+OpenGL3 example", (int)(1280 * main_scale), (int)(800 * main_scale), window_flags);
	if(g_window == nullptr) {
		printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	gl_context = SDL_GL_CreateContext(g_window);
	if(gl_context == nullptr) {
		printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
		return SDL_APP_FAILURE;
	}

	SDL_GL_MakeCurrent(g_window, gl_context);
	SDL_GL_SetSwapInterval(Settings::EnableVsync); // Enable vsync
	SDL_SetWindowPosition(g_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_ShowWindow(g_window);

	#pragma endregion

	#pragma region ImGui Init
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	// io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	// io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	// io.ConfigViewportsNoAutoMerge = true;
	// io.ConfigViewportsNoTaskBarIcon = true;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// Setup scaling
	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(main_scale);   // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
	style.FontScaleDpi = main_scale;   // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)
	io.ConfigDpiScaleFonts = true;	   // [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when Monitor DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
	io.ConfigDpiScaleViewports = true; // [Experimental] Scale Dear ImGui and Platform Windows when Monitor DPI changes.

	if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	// Setup Platform/Renderer bindings
	ImGui_ImplSDL3_InitForOpenGL(g_window, gl_context);
	ImGui_ImplOpenGL3_Init();
	#pragma endregion

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void* appstate) {
	if(shouldQuit) return SDL_APP_SUCCESS;
	ImGuiIO& io = ImGui::GetIO();

	// Start the Dear ImGui frame
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();

	static auto lastTime = std::chrono::high_resolution_clock::now();

	auto now = std::chrono::high_resolution_clock::now();
	auto dt = now - lastTime;

	// might change for different systems
	const int targetFramerate = 60;
	auto frameTime = std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(std::chrono::duration<double>(1.0 / targetFramerate));

	// otherwise we have to manually time the render loop
	if(dt >= frameTime) {
		if(dt >= 2 * frameTime) {
			// dropped frame
			// too much drift so we reset the timer
			lastTime = now;
			logger.Log("Dropped frame\n");
		} else {
			lastTime += frameTime;
		}

		if(emulationCore != nullptr && (running || step)) {
			if(speedUp) {
				auto start = std::chrono::high_resolution_clock::now();

				// run as many ticks as possible in 12 milliseconds
				while((std::chrono::high_resolution_clock::now() - start) < std::chrono::milliseconds(12)) {
					emulationCore->Update();
				}
			} else {
				emulationCore->Update();
			}
			Audio::Resample();
			step = false;
		}
        handleGuiInput();
        Input::Mapper::NewFrame();
	}

	drawGui();


	// Rendering
	ImGui::Render();
	glViewport(0, 0, (int)io.DisplaySize.x, (int)io.DisplaySize.y);
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// Update and Render additional Platform Windows
	// (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
	//  For this specific demo app we could also call SDL_GL_MakeCurrent(window, gl_context) directly)
	if(io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
		SDL_Window* backup_current_window = SDL_GL_GetCurrentWindow();
		SDL_GLContext backup_current_context = SDL_GL_GetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		SDL_GL_MakeCurrent(backup_current_window, backup_current_context);
	}

	SDL_GL_SwapWindow(g_window);
	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
	ImGui_ImplSDL3_ProcessEvent(event);

	if(event->type == SDL_EVENT_QUIT)
		return SDL_APP_SUCCESS;

	// main window
	if(event->key.windowID == SDL_GetWindowID(g_window)) {
		if(event->type == SDL_EVENT_MOUSE_MOTION) {
			lastMouseMove = std::chrono::steady_clock::now();
		}
		if(event->type == SDL_EVENT_DROP_FILE) {
			logger.Log("SDL_EVENT_DROP_FILE %s %s\n", event->drop.source, event->drop.data);

			if(event->drop.data != nullptr) {
				OpenFile(event->drop.data);
			}
		}

		if(event->type == SDL_EVENT_WINDOW_CLOSE_REQUESTED)
			return SDL_APP_SUCCESS;
		if(event->type == SDL_EVENT_KEY_DOWN) {
			Input::Mapper::HandleKeyDown(event->key);
		}
		if(event->type == SDL_EVENT_KEY_UP) {
			Input::Mapper::HandleKeyUp(event->key);

			if(event->key.scancode == SDL_SCANCODE_F12) {
				metricsWindow = !metricsWindow;
			}
		}
	}

	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
	Audio::Dispose();
	Settings::Save();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();

	SDL_GL_DestroyContext(gl_context);
	SDL_DestroyWindow(g_window);
	SDL_Quit();
}
