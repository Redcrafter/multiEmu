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

#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <iostream>
#include <mutex>
#include <bass.h>
#include <filesystem>

bool runningTas = false;
std::shared_ptr<TasController> controller1, controller2;
std::shared_ptr<Bus> nes;
int selectedSaveState = 0;
saver* saveStates[10]{nullptr};

GLuint programID;
GLuint vaoID, vertexBufferID, uvBufferID;
GLuint MatrixLocation;

RenderImage* textures;
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

enum TextureID {
	MainTexture,
	LeftPattern,
	RightPattern,
	AudioTexture
};

bool drawPattern = false, // TODO: mmc3 stuff
     drawAudio = false;
int displayPallet = 0;

bool speedUp = false;
bool running = false;
bool step = false;

HSTREAM audioStreamHandle;

void DrawTexture(TextureID id) {
	auto& texture = textures[id];

	texture.BufferImage();
	auto mat = projection * texture.GetMatrix();
	glUniformMatrix4fv(MatrixLocation, 1, GL_FALSE, &mat[0][0]);
	glDrawArrays(GL_TRIANGLES, 0, 2 * 3);

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
		/*{
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
		}*/
	}
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

		if(drawPattern) {
			nes->ppu.DrawPatternTable(&textures[LeftPattern], 0, displayPallet);
			nes->ppu.DrawPatternTable(&textures[RightPattern], 1, displayPallet);
		}

		step = false;
		Resample();
	}

	DrawTexture(MainTexture);

	if(drawPattern) {
		DrawTexture(LeftPattern);
		DrawTexture(RightPattern);
	}
	if(drawAudio) {
		DrawTexture(AudioTexture);
	}
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
			displayPallet = (displayPallet + 1) % 8;
			break;
		case Action::Step:
			step = true;
			running = false;
			break;
		case Action::ResumeRun:
			running = true;
			break;
		case Action::Reset:
			if(nes != nullptr) {
				nes->Reset();
			}
			break;
		case Action::SaveState:
			if(nes != nullptr) {
				const auto state = new saver();
				delete saveStates[selectedSaveState];
				saveStates[selectedSaveState] = state;
				nes->SaveState(*state);

				try {
					auto path = "./saves/" + nes->cartridge->hash->ToString() + "-" + std::to_string(selectedSaveState) + ".sav";
					if(std::filesystem::exists(path)) {
						std::filesystem::copy(path, path + ".old", std::filesystem::copy_options::overwrite_existing);
					}
					state->Save(path);

					printf("Saved state %i\n", selectedSaveState);
				} catch(std::exception& e) {
					printf("Error saving %s\n", e.what());
				}
			}
			break;
		case Action::LoadState:
			if(nes->cartridge != nullptr) {
				auto state = saveStates[selectedSaveState];
				if(state != nullptr) {
					state->readPos = 0;
					nes->LoadState(*state);

					printf("Loaded state %i\n", selectedSaveState);
				} else {
					printf("Load failed: no state %i\n", selectedSaveState);
				}
			}
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
	projection = glm::ortho(0.0f, (float)width, (float)height, 0.0f);
	glViewport(0, 0, width, height);
}

void onDrop(GLFWwindow* window, int count, const char** paths) {
	std::string path(paths[0]);
	std::shared_ptr<Cartridge> cart;

	try {
		cart = std::make_shared<Cartridge>(path);
	} catch(std::exception& e) {
		printf("Error loading %s\n%s\n", path.c_str(), e.what());
		return;
	}

	if(nes == nullptr) {
		nes = std::make_shared<Bus>(cart);

		nes->controller1 = std::make_shared<StandardController>(0);
		nes->controller2 = std::make_shared<StandardController>(1);
		
		nes->ppu.texture = &textures[0];
	} else {
		nes->InsertCartridge(cart);
		nes->Reset();
	}

	running = true;

	std::string partial = "./saves/" + nes->cartridge->hash->ToString() + "-";

	for(int i = 0; i < 10; ++i) {
		std::string sPath = partial + std::to_string(i) + ".sav";

		if(std::filesystem::exists(sPath)) {
			try {
				saveStates[i] = new saver(sPath);
			} catch(std::exception& e) {
				printf("Error loading state: %s\n%s\n", sPath.c_str(), e.what());
			}
		}
	}
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
	const int scale = 2;

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
	auto window = glfwCreateWindow(293 * scale, 240 * scale, "NES emulator", NULL, NULL);
	if(window == NULL) {
		fprintf(stderr, "Failed to open GLFW window\n");
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);
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

	onResize(window, 293 * scale, 240 * scale);
	#pragma endregion

	#pragma region opengl Init
	programID = LoadShaders("shader.vs", "shader.fs");
	glUseProgram(programID);

	textures = new RenderImage[4]{
		RenderImage(256, 240, 0, 0),                     // Main image
		RenderImage(128, 128, 0, 480),                   // Left pattern table
		RenderImage(128, 128, 128, 480),                 // Right pattern table
		RenderImage(256, 256, 256 * 2 * (8.0 / 7.0), 0), // Audio texture
	};

	textures[0].mat = textures[0].mat * glm::scale(glm::vec3(2 * (8.0 / 7.0), 2, 1.0f));

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

	#pragma region render loop
	do {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		Draw();

		glfwSwapBuffers(window);
		glfwPollEvents();
	} while(glfwWindowShouldClose(window) == 0);
	#pragma endregion

	for(auto saveState : saveStates) {
		delete saveState;
	}
	delete[] textures;
	BASS_Free();

	return 0;
}
