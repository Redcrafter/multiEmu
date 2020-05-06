#include <iostream>
#include <deque>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <nlohmann/json.hpp>

#include <imgui.h>
#include <examples/imgui_impl_opengl3.h>
#include <examples/imgui_impl_glfw.h>

#include "nativefiledialog/nfd.h"
#include "fs.h"

#include "RenderImage.h"
#include "tas.h"
#include "saver.h"

#include "Emulation/Bus.h"
#include "Emulation/Cartridge.h"

#include "Input/TasController.h"
#include "Input/StandardController.h"
#include "Input/Input.h"

#include "imguiWindows/imgui_memory_editor.h"
#include "imguiWindows/imgui_tas_editor.h"
#include "imguiWindows/imgui_pattern_tables.h"
#include "imguiWindows/imgui_cpu_state.h"
#include "imguiWindows/imgui_apu_window.h"
#include "Emulation/Mappers/NsfMapper.h"

#include "logger.h"
#include "audio.h"


bool runningTas = false;
std::shared_ptr<TasController> controller1, controller2;
std::unique_ptr<Bus> nes = nullptr;
int selectedSaveState = 0;
std::array<std::unique_ptr<saver>, 10> saveStates;

std::unique_ptr<RenderImage> mainTexture = nullptr;

GLFWwindow* window = nullptr;

MemoryEditor memEdit{"Memory Editor"};
// TasEditor tasEdit{"Tas Editor"};
PatternTables tables{"Pattern Tables"};
CpuStateWindow cpuWindow{"Cpu State"};
ApuWindow apuWindow{"Apu Visuals"};
bool settingsWindow = false;
bool metricsWindow = false;

struct Settings {
	bool EnableVsync = false;
	int windowScale = 2;
	
	std::deque<std::string> RecentFiles;

	void Load() {
		nlohmann::json j;

		if(fs::exists("./settings.json")) {
			try {
				std::ifstream file("./settings.json");
				file >> j;

				EnableVsync = j["enableVsync"];
				windowScale = j["windowScale"];
				j["recent"].get_to(RecentFiles);
			} catch(std::exception& e) {
				
			}
		}
		
		Input::Load(j);
	}

	void Save() {
		nlohmann::json j = {
			{"enableVsync", EnableVsync},
			{"windowScale", windowScale},
			{"recent", RecentFiles},
		};
		Input::Save(j);

		std::ofstream file("./settings.json");
		file << j;
	}

	void AddRecent(const std::string path) {
		for(int i = 0; i < RecentFiles.size(); ++i) {
			if(RecentFiles[i] == path) {
				RecentFiles.erase(RecentFiles.begin() + i);
				break;
			}
		}
		
		if(RecentFiles.size() >= 10) {
			RecentFiles.pop_back();
		}

		RecentFiles.push_front(path);
	}
} settings;

bool speedUp = false;
bool running = false;
bool step = false;

static void LoadRom(const std::string& path) {
	std::shared_ptr<Mapper> cart;

	try {
		cart = LoadCart(path);
	} catch(std::exception& e) {
		logger.Log("Failed to load rom: %s\n", e.what());
		return;
	}

	settings.AddRecent(path);

	if(nes == nullptr) {
		nes = std::make_unique<Bus>(cart);

		nes->controller1 = std::make_shared<StandardController>(0);
		nes->controller2 = std::make_shared<StandardController>(1);

		nes->ppu.texture = mainTexture.get();

		memEdit.bus = nes.get();
		tables.ppu = &nes->ppu;
		cpuWindow.cpu = &nes->cpu;
		apuWindow.apu = &nes->apu;
	} else {
		nes->apu.vrc6 = false;

		nes->InsertCartridge(cart);
		nes->Reset();
	}

	running = true;

	std::string partial = "./saves/" + nes->cartridge->hash.ToString() + "-";

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

static void LoadNsf(const std::string& path) {
	const auto mapper = std::make_shared<NsfMapper>(path);

	settings.AddRecent(path);

	// if(nes == nullptr) {
	std::shared_ptr<Mapper> ptr = mapper;
	nes = std::make_unique<Bus>(ptr);

	nes->controller1 = std::make_shared<StandardController>(0);
	nes->controller2 = std::make_shared<StandardController>(1);

	nes->ppu.Control.enableNMI = true;

	nes->ppu.texture = mainTexture.get();

	memEdit.bus = nes.get();
	tables.ppu = &nes->ppu;
	cpuWindow.cpu = &nes->cpu;
	apuWindow.apu = &nes->apu;
	/*} else {
		nes->InsertCartridge(cart);
		nes->Reset();
	}*/

	if(mapper->nsf.extraSoundChip.vrc6) {
		nes->apu.vrc6 = true;
	}
	if(mapper->nsf.extraSoundChip.vrc7) {
		logger.Log("vrc7 not supported\n");
	}
	if(mapper->nsf.extraSoundChip.fds) {
		logger.Log("FDS not supported\n");
	}
	if(mapper->nsf.extraSoundChip.mmc5) {
		logger.Log("MMC5 not supported\n");
	}
	if(mapper->nsf.extraSoundChip.namco163) {
		logger.Log("Namoc 163 not supported\n");
	}
	if(mapper->nsf.extraSoundChip.sunsoft5B) {
		logger.Log("Sunsoft 5B not supported\n");
	}

	running = true;
}

static void OpenFile(const std::string& path) {
	const auto asdf = fs::path(path);
	const auto ext = asdf.extension();

	if(ext == L".nes") {
		LoadRom(path);
	} else if(ext == L".nsf") {
		LoadNsf(path);
	} else if(ext == L".fm2") {
		if(!nes) {
			return;
		}

		runningTas = true;
		auto inputs = TasInputs::LoadFM2(path);
		nes->Reset();
		nes->controller1 = controller1 = std::make_shared<TasController>(inputs.Controller1);
		nes->controller2 = controller2 = std::make_shared<TasController>(inputs.Controller2);
	}
}

static void SaveState(int number) {
	if(nes == nullptr) {
		return;
	}
	saveStates[number] = std::make_unique<saver>();
	auto& state = saveStates[number];
	nes->SaveState(*state);

	try {
		if(!fs::exists("./saves/")) {
			fs::create_directory("./saves/");
		}
		auto path = "./saves/" + nes->cartridge->hash.ToString() + "-" + std::to_string(number) + ".sav";
		if(fs::exists(path)) {
			fs::copy(path, path + ".old", fs::copy_options::overwrite_existing);
		}
		state->Save(path);

		logger.Log("Saved state %i\n", number);
	} catch(std::exception& e) {
		logger.Log("Error saving %s\n", e.what());
	}
}

static void LoadState(int number) {
	if(nes->cartridge == nullptr) {
		return;
	}

	const auto& state = saveStates[number];
	if(state != nullptr) {
		state->readPos = 0;
		nes->LoadState(*state);

		logger.Log("Loaded state %i\n", number);
	} else {
		logger.Log("Failed to load state %i\n", number);
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

static void nesFrame() {
	if(!nes || !running && !step)
		return;

	int mapped = 1;
	if(speedUp) {
		mapped = 10;
	}

	for(int i = 0; i < mapped; ++i) {
		// int count = 0; = 89342 
		while(!nes->ppu.frameComplete) {
			nes->Clock();
			// count++;
		}
		// printf("Cycles %i\n", count); 
		nes->ppu.frameComplete = false;

		if(runningTas) {
			controller1->Frame();
			controller2->Frame();
		}
	}

	step = false;
	Audio::Resample(nes->apu);
}

static void onKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
	Input::OnKey(key, scancode, action, mods);

	if(action != GLFW_PRESS) {
		return;
	}

	if(key == GLFW_KEY_F12) {
		metricsWindow = !metricsWindow;
	}

	Action mapped;
	if(!Input::TryGetAction(key, mods, mapped)) {
		return;
	}

	switch(mapped) {
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
			if(nes) {
				nes->Reset();
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
	}
}

static void onResize(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

static void onDrop(GLFWwindow* window, int count, const char** paths) {
	auto asdf = fs::path(paths[0]);

	if(!is_regular_file(asdf)) {
		return;
	}

	OpenFile(paths[0]);
}

static void onGlfwError(int error, const char* description) {
	logger.Log("Glfw Error: %d: %s\n", error, description);
}

static ImVec2 CalcWindowSize() {
	return ImVec2(256.0 * (8.0 / 7.0) * settings.windowScale, 256.0 * settings.windowScale);
}

static void drawSettings(bool& display) {
	if(!display)
		return;

	if(!ImGui::Begin("Settings", &display)) {
		ImGui::End();
		return;
	}
	if(ImGui::BeginTabBar("tabBar")) {
		if(ImGui::BeginTabItem("General")) {
			bool old = settings.EnableVsync;
			ImGui::Checkbox("Vsync", &settings.EnableVsync);
			HelpMarker("Toggle Vsync.\nReduces cpu usage on fast cpu's but can cause stuttering");
			if(settings.EnableVsync != old) {
				glfwSwapInterval(settings.EnableVsync);
			}

			int val = settings.windowScale - 1;
			static const char* drawModeNames[] = { "x1", "x2", "x3", "x4" };
			if(ImGui::Combo("DrawMode", &val, drawModeNames, 4)) {
				settings.windowScale = val + 1;
				auto s = CalcWindowSize();
				glfwSetWindowSize(window, s.x, s.y);
			}
			ImGui::EndTabItem();
		}

		if(ImGui::BeginTabItem("Inputs")) {
			ImGui::BeginChild("inputs");
			Input::ShowEditWindow();
			ImGui::EndChild();

			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	ImGui::End();
}

// fix for menubar closing when menu is too big and creates new context
static bool menuOpen = false;

static void drawGui() {
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoScrollbar;

	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->GetWorkPos());
	ImGui::SetNextWindowSize(viewport->GetWorkSize());
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	#ifdef GLFW_HOVERED
	if(menuOpen || glfwGetWindowAttrib(window, GLFW_HOVERED)) {
		window_flags |= ImGuiWindowFlags_MenuBar;
	}
	#else
	window_flags |= ImGuiWindowFlags_MenuBar;
	#endif

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("test", nullptr, window_flags);
	ImGui::PopStyleVar();

	ImGui::PopStyleVar(2);

	auto size = ImGui::GetWindowSize();

	if(size.x * (7.0 / 8.0) < size.y) {
		size.y = size.x * (7.0 / 8.0);
	} else {
		size.x = size.y * (8.0 / 7.0);
	}

	auto test = ImGui::GetWindowSize();
	test.x = (test.x - size.x) * 0.5;
	test.y = (test.y - size.y) * 0.5;
	
	ImGui::SetCursorPos(test);

	mainTexture->BufferImage();
	ImGui::Image((void*)mainTexture->GetTextureId(), size);

	bool enabled = nes != nullptr;
	menuOpen = false;
	if(ImGui::BeginMenuBar()) {
		if(ImGui::BeginMenu("File")) {
			menuOpen = true;
			
			if(ImGui::MenuItem("Open ROM", "CTRL+O")) {
				std::string outPath;
				const auto res = NFD::OpenDialog("nes,nfs", nullptr, outPath);

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
					settings.RecentFiles.clear();
				}

				ImGui::EndMenu();
			}

			ImGui::Separator();

			// char temp[2] { 0, 0 };
			// char asdf[] = "Shift+";
			if(ImGui::BeginMenu("Save State", enabled)) {
				for(int i = 0; i < 10; ++i) {
					std::string str = std::to_string(i);
					// temp[0] = i + 48;
					
					if(ImGui::MenuItem(str.c_str(), ("Shift+" + str).c_str())) {
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
				if(nes) {
					nes->Reset();
				}
			}
			if(ImGui::MenuItem("Hard Reset", nullptr, false, enabled)) {
				nes = nullptr;
				OpenFile(settings.RecentFiles[0]);
			}

			ImGui::EndMenu();
		}

		if(ImGui::BeginMenu("Tools")) {
			menuOpen = true;

			bool active = nes != nullptr;

			if(ImGui::MenuItem("Settings")) {
				settingsWindow = true;
			}
			if(ImGui::MenuItem("Hex Editor", nullptr, false, active)) {
				memEdit.Open();
			}
			/*if(ImGui::MenuItem("Tas Editor", nullptr, false, active)) {
				tasEdit.Open();
			}*/
			if(ImGui::MenuItem("CPU Viewer", nullptr, false, active)) {
				cpuWindow.Open();
			}
			if(ImGui::MenuItem("PPU Viewer", nullptr, false, active)) {
				tables.Open();
			}
			if(ImGui::MenuItem("APU Visuals", nullptr, false, active)) {
				apuWindow.Open();
			}
			if(ImGui::MenuItem("Log")) {
				logger.Show = true;
			}

			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	ImGui::End();
	
	cpuWindow.DrawWindow();
	tables.DrawWindow();
	// tasEdit.DrawWindow(0);
	memEdit.DrawWindow();
	apuWindow.DrawWindow();
	logger.Draw();

	if(metricsWindow) {
		ImGui::ShowMetricsWindow(&metricsWindow);
	}
	drawSettings(settingsWindow);
}

#ifdef WIN32
#include <windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
#else
int main() {
#endif
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
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL

	// TODO: glfwWindowHint(GLFW_DECORATED, false);
	
	auto s = CalcWindowSize();
	// Open a window and create its OpenGL context
	window = glfwCreateWindow(s.x, s.y, "NES emulator", nullptr, nullptr);
	if(window == nullptr) {
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(settings.EnableVsync);

	// Initialize GLEW
	if(glewInit() != GLEW_OK) {
		logger.Log("Failed to initialize GLEW\n");
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	glfwSetKeyCallback(window, onKey); // TODO: support controllers
	glfwSetWindowSizeCallback(window, onResize);
	glfwSetDropCallback(window, onDrop);

	onResize(window, s.x, s.y);

	mainTexture = std::make_unique<RenderImage>(256, 240);
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
	ImGui_ImplOpenGL3_Init("#version 330 core");

	tables.Init();
	#pragma endregion

	auto lastTime = glfwGetTime();
	
	#pragma region render loop
	do {
		#pragma region Timing
		auto time = glfwGetTime();
		auto dt = time - lastTime;

		if(dt >= 2 / 60.0) {
			// dropped frame
			lastTime = time;
		} else if(dt >= 1 / 60.0) {
			lastTime += 1 / 60.0;
		} else {
			glfwPollEvents();
			continue;
		}
		#pragma endregion

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::ShowDemoWindow();

		nesFrame();
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
