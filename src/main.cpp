#include <iostream>
#include <deque>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include <nlohmann/json.hpp>
#include <RtAudio.h>

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

#include "shaders/shader.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_glfw.h"

#include "imguiWindows/imgui_memory_editor.h"
#include "imguiWindows/imgui_tas_editor.h"
#include "imguiWindows/imgui_pattern_tables.h"
#include "imguiWindows/imgui_cpu_state.h"
#include "imguiWindows/imgui_apu_window.h"

bool runningTas = false;
std::shared_ptr<TasController> controller1, controller2;
std::shared_ptr<Bus> nes = nullptr;
int selectedSaveState = 0;
saver* saveStates[10]{};

GLuint programID;
GLuint vaoID, vertexBufferID, uvBufferID;
GLuint MatrixLocation;

RenderImage* mainTexture;
glm::mat4 projection;

const char* const DrawModeNames[] = {"AutoSize", "x1", "x2", "x3", "x4", "Window"};

enum DrawMode {
	Auto = 0,
	x1 = 1,
	x2 = 2,
	x3 = 3,
	x4 = 4,
	Window = 5,
};

static const GLfloat vertexBuffer[] = {
	0, 0, 0,
	1, 0, 0,
	1, 1, 0,

	0, 0, 0,
	1, 1, 0,
	0, 1, 0,
};
static const GLfloat uvBuffer[] = {
	0, 0,
	1, 0,
	1, 1,

	0, 0,
	1, 1,
	0, 1,
};

MemoryEditor memEdit{"Memory Editor"};
TasEditor tasEdit{"Tas Editor"};
PatternTables tables{"Pattern Tables"};
CpuStateWindow cpuWindow{"Cpu State"};
ApuWindow apuWindow{"Apu Visuals"};
bool settingsWindow = false;
bool metricsWindow = false;

struct Settings {
	bool EnableVsync = false;
	DrawMode drawMode = Auto;

	std::deque<std::string> RecentRoms;

	void Load() {
		nlohmann::json j;

		if(fs::exists("./settings.json")) {
			std::ifstream file("./settings.json");
			file >> j;

			EnableVsync = j["enableVsync"];
			drawMode = j["drawMode"];
			j["recent"].get_to(RecentRoms);
		}

		Input::Load(j);
	}

	void Save() {
		nlohmann::json j = {
			{"enableVsync", EnableVsync},
			{"drawMode", drawMode},
			{"recent", RecentRoms},
		};
		Input::Save(j);

		std::ofstream file("./settings.json");
		file << j;
	}
} settings;

bool speedUp = false;
bool running = false;
bool step = false;

int WindowWidth = 512 * (8.0 / 7.0);
int WindowHeight = 512;

int samples = 0;
uint64_t readBuf = 0, writeBuf = 0;
uint32_t readPos = 0;
float buffer[8][735];

void LoadRom(const char* path) {
	std::shared_ptr<Cartridge> cart;

	try {
		cart = std::make_shared<Cartridge>(path);
	} catch(std::exception& e) {
		printf("Error loading %s\n%s\n", path, e.what());
		return;
	}

	for(int i = 0; i < settings.RecentRoms.size(); ++i) {
		if(settings.RecentRoms[i] == path) {
			settings.RecentRoms.erase(settings.RecentRoms.begin() + i);
			break;
		}
	}

	if(settings.RecentRoms.size() >= 10) {
		settings.RecentRoms.pop_back();
	}

	settings.RecentRoms.push_front(path);

	if(nes == nullptr) {
		nes = std::make_shared<Bus>(cart);

		nes->controller1 = std::make_shared<StandardController>(0);
		nes->controller2 = std::make_shared<StandardController>(1);

		nes->ppu.texture = mainTexture;

		memEdit.bus = nes.get();
		tables.ppu = &nes->ppu;
		cpuWindow.cpu = &nes->cpu;
		apuWindow.apu = &nes->apu;
	} else {
		nes->InsertCartridge(cart);
		nes->Reset();
	}

	running = true;

	std::string partial = "./saves/" + nes->cartridge->hash->ToString() + "-";

	for(int i = 0; i < 10; ++i) {
		std::string sPath = partial + std::to_string(i) + ".sav";

		if(fs::exists(sPath)) {
			try {
				saveStates[i] = new saver(sPath);
			} catch(std::exception& e) {
				printf("Error loading state: %s\n%s\n", sPath.c_str(), e.what());
			}
		}
	}
}

void SaveState(int number) {
	if(nes == nullptr) {
		return;
	}
	const auto state = new saver();
	delete saveStates[number];
	saveStates[number] = state;
	nes->SaveState(*state);

	try {
		if(!fs::exists("./saves/")) {
			fs::create_directory("./saves/");
		}
		auto path = "./saves/" + nes->cartridge->hash->ToString() + "-" + std::to_string(number) + ".sav";
		if(fs::exists(path)) {
			fs::copy(path, path + ".old", fs::copy_options::overwrite_existing);
		}
		state->Save(path);

		printf("Saved state %i\n", number);
	} catch(std::exception& e) {
		printf("Error saving %s\n", e.what());
	}
}

void LoadState(int number) {
	if(nes->cartridge == nullptr) {
		return;
	}

	auto state = saveStates[number];
	if(state != nullptr) {
		state->readPos = 0;
		nes->LoadState(*state);

		printf("Loaded state %i\n", number);
	} else {
		printf("Failed to load state %i\n", number);
	}
}

void HelpMarker(const char* desc) {
	// ImGui::TextDisabled("(?)");
	if(ImGui::IsItemHovered()) {
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

void Resample() {
	samples = nes->apu.bufferPos;

	const float frac = samples / 735.0;
	int readPos = 0;
	float mapped = 0;

	for(int writePos = 0; writePos < 735 && readPos < samples; writePos++) {
		float sample = 0;
		int count = 0;

		while(mapped <= frac && readPos < samples) {
			mapped += 1;
			sample += nes->apu.waveBuffer[readPos].sample;
			readPos++;
			count++;
		}
		mapped -= frac;

		buffer[writeBuf & 7][writePos] = (sample / count);
	}
	writeBuf++;
	nes->apu.bufferPos = 0;
}

int AudioCallback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames, double streamTime, RtAudioStreamStatus status, void* userData) {
	auto outBuffer = static_cast<float*>(outputBuffer);

	int i = 0;
	for(; i < nBufferFrames && readBuf < writeBuf; i++) {
		outBuffer[i * 2] = outBuffer[i * 2 + 1] = buffer[readBuf & 7][readPos];
		readPos++;
		if(readPos >= 735) {
			readBuf++;
			readPos = 0;
		}
	}

	if(i < nBufferFrames) {
		// Repeat last sample to prevent popping
		auto last = buffer[(readBuf - 1) & 7][734];
		for(int i = 0; i < nBufferFrames; ++i) {
			outBuffer[i * 2] = outBuffer[i * 2 + 1] = last;
		}
	}

	return 0;
}

void Frame() {
	if(!nes || !running && !step)
		return;

	int mapped = 1;
	if(speedUp) {
		mapped = 5;
	}

	for(int i = 0; i < mapped; ++i) {
		int count = 0;
		while(!nes->ppu.frameComplete) {
			nes->Clock();
			count++;
		}
		nes->ppu.frameComplete = false;

		if(runningTas) {
			controller1->Frame();
			controller2->Frame();
		}
	}

	step = false;
	Resample();
}

void onKey(GLFWwindow* window, int key, int scancode, int action, int mods) {
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
			printf("Selected state %i\n", selectedSaveState);
			break;
		case Action::SelectLastState:
			selectedSaveState -= 1;
			if(selectedSaveState < 0) {
				selectedSaveState += 10;
			}
			printf("Selected state %i\n", selectedSaveState);
			break;
	}
}

void onResize(GLFWwindow* window, int width, int height) {
	WindowWidth = width;
	WindowHeight = height;

	projection = glm::ortho(0.0f, (float)width, (float)height, 0.0f);
	glViewport(0, 0, width, height);
}

void onDrop(GLFWwindow* window, int count, const char** paths) {
	auto asdf = fs::path(paths[0]);

	if(!is_regular_file(asdf)) {
		return;
	}

	auto ext = asdf.extension();
	// std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });

	if(ext == L".nes") {
		LoadRom(paths[0]);
	} else if(ext == L".fm2") {
		if(!nes) {
			return;
		}

		runningTas = true;
		auto inputs = TasInputs::LoadFM2(paths[0]);
		nes->Reset();
		nes->controller1 = controller1 = std::make_shared<TasController>(inputs.Controller1);
		nes->controller2 = controller2 = std::make_shared<TasController>(inputs.Controller2);
	}
}

void onGlfwError(int error, const char* description) {
	fprintf(stderr, "Glfw Error: %d: %s\n", error, description);
}

void CalcWindowSize() {
	if(settings.drawMode == Auto || settings.drawMode == Window) {
		return;
	}

	WindowWidth = 256;;
	WindowHeight = 256;
	switch(settings.drawMode) {
		case Auto:
		case Window: break;
		default:
			WindowWidth *= settings.drawMode;
			WindowHeight *= settings.drawMode;
			break;
	}
	WindowWidth *= 8.0 / 7.0;
	WindowHeight += 19;
}

#ifdef WIN32
#include <windows.h>
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
#else
int main() {
#endif
	try {
		settings.Load();
	} catch(std::exception& e) {
		std::cout << "Failed to load settings: " << e.what() << std::endl;
	}

	#pragma region RtAudio Init
	RtAudio dac;

	if(dac.getDeviceCount() >= 1) {
		RtAudio::StreamParameters parameters;
		parameters.deviceId = dac.getDefaultOutputDevice();
		parameters.nChannels = 2;
		parameters.firstChannel = 0;

		RtAudio::StreamOptions options;
		// options.flags = RTAUDIO_MINIMIZE_LATENCY; // breaks mac TODO: handle non fixed sample request

		unsigned int sampleRate = 44100;
		unsigned int bufferFrames = 735;

		try {
			dac.openStream(&parameters, nullptr, RTAUDIO_FLOAT32, sampleRate, &bufferFrames, &AudioCallback, nullptr, &options);
			dac.startStream();
		} catch(RtAudioError& e) {
			std::cout << __FILE__ << ':' << __LINE__ << ": " << e.what() << std::endl;
		}
	} else {
		std::cout << "No audio devices found!\n";
	}
	#pragma endregion

	#pragma region glfw Init
	glfwSetErrorCallback(onGlfwError);
	if(!glfwInit()) {
		fprintf(stderr, "Failed to initialize GLFW\n");
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);                               // 4x antialiasing
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);                 // We want OpenGL 3.3
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);                 //
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);           // To make MacOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // We don't want the old OpenGL 

	CalcWindowSize();
	// Open a window and create its OpenGL context
	auto window = glfwCreateWindow(WindowWidth, WindowHeight, "NES emulator", NULL, NULL);
	if(window == NULL) {
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSwapInterval(settings.EnableVsync);

	// Initialize GLEW
	if(glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	glfwSetKeyCallback(window, onKey); // TODO: support controllers
	glfwSetWindowSizeCallback(window, onResize);
	glfwSetDropCallback(window, onDrop);

	onResize(window, WindowWidth, WindowHeight);
	#pragma endregion

	#pragma region opengl Init
	programID = LoadShaders();
	glUseProgram(programID);

	mainTexture = new RenderImage(256, 240);
	MatrixLocation = glGetUniformLocation(programID, "MVP");

	glClearColor(0, 0, 0, 0);

	#pragma region Vao
	glGenVertexArrays(1, &vaoID);
	glBindVertexArray(vaoID);

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);

	glGenBuffers(1, &vertexBufferID);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertexBuffer), &vertexBuffer, GL_STATIC_DRAW);
	glVertexAttribPointer(
		0,        // attribute 0. No particular reason for 0, but must match the layout in the shader.
		3,        // size
		GL_FLOAT, // type
		GL_FALSE, // normalized?
		0,        // stride
		nullptr   // array buffer offset
	);

	glGenBuffers(1, &uvBufferID);
	glBindBuffer(GL_ARRAY_BUFFER, uvBufferID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(uvBuffer), &uvBuffer, GL_STATIC_DRAW);
	glVertexAttribPointer(
		1,        // attribute. No particular reason for 1, but must match the layout in the shader.
		2,        // size
		GL_FLOAT, // type
		GL_FALSE, // normalized?
		0,        // stride
		nullptr   // array buffer offset
	);
	#pragma endregion
	#pragma endregion

	#pragma region ImGui Init
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	// (void)io;
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

		Frame();

		#pragma region GUI
		mainTexture->BufferImage();
		if(settings.drawMode == Window) {
			bool open = true;
			if(ImGui::Begin("NES", &open, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar)) {
				auto size = ImGui::GetWindowSize();

				if(size.x * (7.0 / 8.0) < size.y) {
					size.y = size.x * (7.0 / 8.0);
				} else {
					size.x = size.y * (8.0 / 7.0);
				}

				ImGui::Image((void*)mainTexture->GetTextureId(), size);
			}
			ImGui::End();
		} else {
			float Width = 256 * (8.0 / 7.0);
			float Height = 256;
			int X = 0, Y = 0;

			switch(settings.drawMode) {
				case Auto:
					if(WindowWidth * (7.0 / 8.0) < WindowHeight) {
						Width = WindowWidth;
						Height = WindowWidth * (7.0 / 8.0);
						Y = (WindowHeight - Height) / 2;
					} else {
						Height = WindowHeight;
						Width = Height * (8.0 / 7.0);
						X = (WindowWidth - Width) / 2;
					}
					break;
				case x1:
				case x2:
				case x3:
				case x4:
					Width *= settings.drawMode;
					Height *= settings.drawMode;
					break;
			}

			auto mat = projection * (translate(glm::vec3(X, Y, 0.0f)) * scale(glm::vec3(Width, Height, 1.0f)));
			glUniformMatrix4fv(MatrixLocation, 1, GL_FALSE, &mat[0][0]);
			glDrawArrays(GL_TRIANGLES, 0, 2 * 3);
		}

		bool enabled = nes != nullptr;
		if(/*glfwGetWindowAttrib(window, GLFW_HOVERED) &&*/ ImGui::BeginMainMenuBar()) {
			if(ImGui::BeginMenu("File")) {
				if(ImGui::MenuItem("Open ROM", "CTRL+O")) {
					std::string outPath;
					const auto res = NFD::OpenDialog("nes", nullptr, outPath);

					if(res == NFD::Result::Okay) {
						LoadRom(outPath.c_str());
					}
				}
				if(ImGui::BeginMenu("Recent ROMs")) {
					for(int i = 0; i < settings.RecentRoms.size(); ++i) {
						std::string str = settings.RecentRoms[i];

						if(ImGui::MenuItem(str.c_str())) {
							LoadRom(str.c_str());
						}
					}

					ImGui::Separator();
					if(ImGui::MenuItem("Clear")) {
						settings.RecentRoms.clear();
					}

					ImGui::EndMenu();
				}

				ImGui::Separator();

				if(ImGui::BeginMenu("Save State", enabled)) {
					for(int i = 0; i < 10; ++i) {
						std::string str = std::to_string(i);
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
					break;
				}

				ImGui::EndMenu();
			}

			if(ImGui::BeginMenu("Emulation")) {
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
					LoadRom(settings.RecentRoms[0].c_str());
				}

				ImGui::EndMenu();
			}

			if(ImGui::BeginMenu("Tools")) {
				bool active = nes != nullptr;

				if(ImGui::MenuItem("Settings")) {
					settingsWindow = true;
				}
				if(ImGui::MenuItem("Hex Editor", nullptr, false, active)) {
					memEdit.Open();
				}
				if(ImGui::MenuItem("Tas Editor", nullptr, false, active)) {
					tasEdit.Open();
				}
				if(ImGui::MenuItem("CPU Viewer", nullptr, false, active)) {
					cpuWindow.Open();
				}
				if(ImGui::MenuItem("PPU Viewer", nullptr, false, active)) {
					tables.Open();
				}
				if(ImGui::MenuItem("APU Visuals", nullptr, false, active)) {
					apuWindow.Open();
				}

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		cpuWindow.DrawWindow();
		tables.DrawWindow();
		tasEdit.DrawWindow(0);
		memEdit.DrawWindow();
		apuWindow.DrawWindow(samples);

		if(metricsWindow) {
			ImGui::ShowMetricsWindow(&metricsWindow);
		}
		if(settingsWindow) {
			if(ImGui::Begin("Settings", &settingsWindow)) {
				if(ImGui::BeginTabBar("tabBar")) {
					if(ImGui::BeginTabItem("General")) {
						bool old = settings.EnableVsync;
						ImGui::Checkbox("Vsync", &settings.EnableVsync);

						HelpMarker("Toggle Vsync.\nReduces cpu usage on fast cpu's but can cause stuttering");

						if(settings.EnableVsync != old) {
							glfwSwapInterval(settings.EnableVsync);
						}
						if(ImGui::Combo("DrawMode", (int*)&settings.drawMode, DrawModeNames, 6)) {
							CalcWindowSize();
							glfwSetWindowSize(window, WindowWidth, WindowHeight);
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
			}
			ImGui::End();
		}

		#pragma endregion

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	} while(glfwWindowShouldClose(window) == 0);
	#pragma endregion

	settings.Save();

	for(auto saveState : saveStates) {
		delete saveState;
	}
	delete mainTexture;

	try {
		dac.stopStream();
	} catch(RtAudioError& e) {
		std::cout << e.what() << std::endl;
	}

	if(dac.isStreamOpen()) {
		dac.closeStream();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
