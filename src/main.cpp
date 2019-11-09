#include "Emulation/Bus.h"
#include "Emulation/Cartridge.h"
#include "Emulation/ppu2C02.h"
#include "RenderImage.h"

#include "Input/TasController.h"
#include "Input/StandardController.h"
#include "tas.h"
#include "saver.h"
#include "Input.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "shaders/shader.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <iostream>
#include <mutex>
#include <bass.h>
#include "fs.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_opengl3.h"
#include "imgui/imgui_impl_glfw.h"

#include "imguiWindows/imgui_memory_editor.h"
#include "imguiWindows/imgui_tas_editor.h"
#include "imguiWindows/imgui_pattern_tables.h"
#include "imguiWindows/imgui_cpu_state.h"

#include "nativefiledialog/nfd.h"

bool runningTas = false;
std::shared_ptr<TasController> controller1, controller2;
std::shared_ptr<Bus> nes = nullptr;
std::string currentRom;
int selectedSaveState = 0;
saver* saveStates[10]{nullptr};

GLuint programID;
GLuint vaoID, vertexBufferID, uvBufferID;
GLuint MatrixLocation;

RenderImage* mainTexture;
glm::mat4 projection;

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

MemoryEditor memEdit;
TasEditor tasEdit{"Tas Editor"};
PatternTables tables{"Pattern Tables"};
CpuStateWindow cpuWindow{"Cpu State"};

bool speedUp = false;
bool running = false;
bool step = false;

HSTREAM audioStreamHandle;

int gameScale = 2;
int WindowWidth = gameScale * 256;
int WindowHeight = WindowWidth * (7.0 / 8.0) + 19;

void LoadRom(const char* path) {
	std::shared_ptr<Cartridge> cart;

	try {
		cart = std::make_shared<Cartridge>(path);
	} catch(std::exception& e) {
		printf("Error loading %s\n%s\n", path, e.what());
		return;
	}
	currentRom = path;

	if(nes == nullptr) {
		nes = std::make_shared<Bus>(cart);

		nes->controller1 = std::make_shared<StandardController>(0);
		nes->controller2 = std::make_shared<StandardController>(1);

		nes->ppu.texture = mainTexture;
		
		tables.ppu = &nes->ppu;
		cpuWindow.cpu = &nes->cpu;
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

void Resample() {
	static float buffer[735];

	const float requested = 735; // TODO: calculate requested by average frame time
	const int available = nes->apu.bufferPos;

	const float frac = available / requested;
	int readPos = 0;
	float mapped = 0;

	int writePos = 0;
	while(writePos < requested && writePos < sizeof(buffer) && readPos < available) {
		float sample = 0;
		int count = 0;

		while(mapped <= frac && readPos < available) {
			mapped += 1;
			sample += nes->apu.waveBuffer[readPos].sample;
			readPos++;
			count++;
		}
		mapped -= frac;

		buffer[writePos] = (sample / count);
		writePos++;
	}
	nes->apu.bufferPos = 0;

	BASS_StreamPutData(audioStreamHandle, buffer, writePos * 4);

	/*
	if(drawAudio) {
		auto& text = textures[AudioTexture];
		text.Clear({0, 0, 0});

		auto pps = 256.0 / available;
		auto spp = available / 256.0;

		// TODO: maybe search for first change

		// Pulse 1
		{
			int startI = -1;
			for(int i = 0; i < available; ++i) {
				auto val = nes->apu.waveBuffer[i].pulse1;
				if(val == 0) {
					startI = i;
					break;
				}
			}

			if(startI != -1) {
				int lastX = 0;
				int lastY = 0;

				for(int i = startI; i < available; ++i) {
					auto val = nes->apu.waveBuffer[i].pulse1;
					if(lastY != val) {
						auto x = pps * (i - startI);

						text.Line(lastX, lastY * 4 + 2, x, lastY * 4 + 2, {255, 255, 255});
						text.Line(x, lastY * 4 + 2, x, val * 4 + 2, {255, 255, 255});

						lastX = x;
						lastY = val;
					}
				}

				text.Line(lastX, lastY * 4 + 2, 256, lastY * 4 + 2, {255, 255, 255});
			}
		}

		// Pulse 2
		{
			int startI = -1;
			for(int i = 0; i < available; ++i) {
				auto val = nes->apu.waveBuffer[i].pulse2;
				if(val == 0) {
					startI = i;
					break;
				}
			}

			if(startI != -1) {
				int lastX = 0;
				int lastY = 0;

				for(int i = startI; i < available; ++i) {
					auto val = nes->apu.waveBuffer[i].pulse2;
					if(lastY != val) {
						auto x = pps * (i - startI);

						text.Line(lastX, lastY * 4 + 64 + 2, x, lastY * 4 + 64 + 2, {255, 255, 255});
						text.Line(x, lastY * 4 + 64 + 2, x, val * 4 + 64 + 2, {255, 255, 255});

						lastX = x;
						lastY = val;
					}
				}

				text.Line(lastX, lastY * 4 + 64 + 2, 256, lastY * 4 + 64 + 2, {255, 255, 255});
			}
		}

		// Triangle
		{
			int startI = -1;
			for(int i = 0; i < available; ++i) {
				auto val = nes->apu.waveBuffer[i].triangle;
				if(val == 0) {
					startI = i;
					break;
				}
			}

			if(startI != -1) {
				int lastX = 0;
				int lastY = 0;

				for(int x = 0; x < 256; ++x) {
					auto val = nes->apu.waveBuffer[static_cast<int>(x * spp) + startI].triangle;

					if(lastY != val) {
						text.Line(lastX, lastY + 128 + 2, x, lastY + 128 + 2, {255, 255, 255});
						text.Line(x, lastY + 128 + 2, x, val + 128 + 2, {255, 255, 255});

						lastX = x;
						lastY = val;
					}
				}
				// TODO: draw to end
			}
		}

		// TODO: Noise and DMC
		// Noise
		{
			int lastX = 0;
			int lastY = 0;

			for(int i = 0; i < available; ++i) {
				auto val = nes->apu.waveBuffer[i].noise;
				if(lastY != val) {
					auto x = size * i;

					text.Line(lastX, lastY * 4 + 192 + 2, x, lastY * 4 + 192 + 2, {255, 255, 255});
					text.Line(x, lastY * 4 + 192 + 2, x, val * 4 + 192 + 2, {255, 255, 255});

					lastX = x;
					lastY = val;
				}
			}
		}
	}*/
}

void Draw() {
	if(nes && (running || step)) {
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
				// std::cout << frameCounter << ": " << controller1->GetInput() << "|" << controller2->GetInput() << std::endl;
				controller1->Frame();
				controller2->Frame();
			}
		}

		step = false;
		Resample();
	}

	int Width, Height, X = 0, Y = 19;
	#if false
	if(WindowWidth < WindowHeight * (8.0 / 7.0) - 19) {
		Width = WindowWidth;
		Height = Width * (7.0 / 8.0);
	} else {
		Height = WindowHeight - 19;
		Width = Height * (8.0 / 7.0);
		X = (WindowWidth - Width) / 2;
	}
	#else
	Width = 512;
	Height = Width * (7.0 / 8.0);
	#endif

	mainTexture->BufferImage();
	auto mat = projection * (translate(glm::vec3(X, Y, 0.0f)) * scale(glm::vec3(Width, Height, 1.0f)));
	glUniformMatrix4fv(MatrixLocation, 1, GL_FALSE, &mat[0][0]);
	glDrawArrays(GL_TRIANGLES, 0, 2 * 3);
}

void onKeyCall(GLFWwindow* window, int key, int scancode, int action, int mods) {
	Input::OnKey(key, scancode, action, mods);

	if(action != GLFW_PRESS) {
		return;
	}

	Action mapped;
	if(!Input::TryGetAction(key, mapped)) {
		return;
	}

	switch(mapped) {
		case Action::Speedup:
			speedUp = !speedUp;
			break;
		case Action::ChangePallet:
			// displayPallet = (displayPallet + 1) % 8;
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
	LoadRom(paths[0]);
}

void onGlfwError(int error, const char* description) {
	fprintf(stderr, "Glfw Error: %d: %s\n", error, description);
}

int main() {
	Input::LoadKeyMap();

	#pragma region Bass Init
	{
		BASS_SetConfig(BASS_CONFIG_VISTA_TRUEPOS, 0);
		BASS_SetConfig(BASS_CONFIG_UPDATEPERIOD, 10);

		if(!BASS_Init(-1, 44100, BASS_DEVICE_LATENCY, 0, NULL)) {
			std::cerr << "Failed to initialize audio" << std::endl;
			return -1;
		}

		BASS_INFO info;
		BASS_GetInfo(&info);
		BASS_SetConfig(BASS_CONFIG_BUFFER, 10 + info.minbuf + 1);

		audioStreamHandle = BASS_StreamCreate(44100, 1, BASS_SAMPLE_FLOAT, STREAMPROC_PUSH, nullptr);

		// nes->apu.streamHandle = streamHandle;
		BASS_ChannelPlay(audioStreamHandle, FALSE);
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

	// Open a window and create its OpenGL context
	auto window = glfwCreateWindow(WindowWidth, WindowHeight, "NES emulator", NULL, NULL);
	if(window == NULL) {
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
	// glfwSwapInterval(1); // Enable vsync

	// Initialize GLEW
	if(glewInit() != GLEW_OK) {
		fprintf(stderr, "Failed to initialize GLEW\n");
		return -1;
	}
	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	glfwSetKeyCallback(window, onKeyCall); // TODO: support controllers
	glfwSetWindowSizeCallback(window, onResize);
	glfwSetDropCallback(window, onDrop);

	onResize(window, WindowWidth, WindowHeight);
	#pragma endregion

	#pragma region opengl Init
	programID = LoadShaders("shader.vs", "shader.fs");
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
	(void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 330 core");

	tables.Init();

	memEdit.Open = false;
	memEdit.ReadFn = [](const uint8_t* data, uint64_t addr) {
		uint8_t res = 0;

		if(nes) {
			res = nes->CpuRead(addr, true);
		}

		return res;
	};
	memEdit.WriteFn = [](uint8_t* data, uint64_t addr, uint8_t value) {

	};
	#pragma endregion

	#pragma region render loop
	do {
		bool enabled = nes != nullptr;

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// ImGui::ShowDemoWindow();

		Draw();

		if(ImGui::BeginMainMenuBar()) {
			if(ImGui::BeginMenu("File")) {
				if(ImGui::MenuItem("Open ROM", "CTRL+O")) {
					char* outPath = nullptr;
					const auto res = NFD_OpenDialog({"nes"}, nullptr, &outPath);

					if(res == NFD_OKAY) {
						LoadRom(outPath);
					}
				}
				if(ImGui::BeginMenu("Recent ROM")) {

					ImGui::Separator();
					if(ImGui::MenuItem("Clear")) {

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
						if(ImGui::MenuItem(str.c_str(), ("Shift+" + str).c_str())) {
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
					LoadRom(currentRom.c_str());
				}

				ImGui::EndMenu();
			}

			if(ImGui::BeginMenu("Tools")) {
				if(ImGui::MenuItem("RAM Watch")) {
					memEdit.Open = true;
				}

				if(ImGui::MenuItem("Tas Editor")) {
					tasEdit.Open();
				}

				if(ImGui::MenuItem("CPU Viewer")) {
					cpuWindow.Open();
				}
				if(ImGui::MenuItem("PPU Viewer")) {
					tables.Open();
				}

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		cpuWindow.DrawWindow();
		tables.DrawWindow();
		tasEdit.DrawWindow();
		
		if(memEdit.Open) {
			memEdit.DrawWindow("Memory Editor", nullptr, 0x10000);
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(window);
		glfwPollEvents();
	} while(glfwWindowShouldClose(window) == 0);
	#pragma endregion

	for(auto saveState : saveStates) {
		delete saveState;
	}
	delete mainTexture;

	BASS_Free();

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
