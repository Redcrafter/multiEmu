#include "NesCore.h"

#include <algorithm>

#include "../../Input.h"
#include "../../logger.h"
#include "../../fs.h"

#include "Mappers/NsfMapper.h"
#include "StandardController.h"

namespace Nes {

Core::Core() : texture(256, 240) {
	LoadCardDb("./NesCarts (2017-08-21).json");

	Input::SetMapper(Input::InputMapper("Nes", {
		{ "Controller1 A",      0,  {{ GLFW_KEY_A,     0 }} },
		{ "Controller1 B",      1,  {{ GLFW_KEY_B,     0 }} },
		{ "Controller1 Start",  2,  {{ GLFW_KEY_S,     0 }} },
		{ "Controller1 Select", 3,  {{ GLFW_KEY_ENTER, 0 }} },
		{ "Controller1 Up",     4,  {{ GLFW_KEY_UP,    0 }} },
		{ "Controller1 Down",   5,  {{ GLFW_KEY_DOWN,  0 }} },
		{ "Controller1 Left",   6,  {{ GLFW_KEY_LEFT,  0 }} },
		{ "Controller1 Right",  7,  {{ GLFW_KEY_RIGHT, 0 }} },

		{ "Controller2 A",      8,  {{ 0, 0 }} },
		{ "Controller2 B",      9,  {{ 0, 0 }} },
		{ "Controller2 Start",  10, {{ 0, 0 }} },
		{ "Controller2 Select", 11, {{ 0, 0 }} },
		{ "Controller2 Up",     12, {{ 0, 0 }} },
		{ "Controller2 Down",   13, {{ 0, 0 }} },
		{ "Controller2 Left",   14, {{ 0, 0 }} },
		{ "Controller2 Right",  15, {{ 0, 0 }} },
	}));

	emulator.controller1 = std::make_shared<StandardController>(0);
	emulator.controller2 = std::make_shared<StandardController>(1);
	emulator.ppu.texture = &texture;

	tables.ppu = &emulator.ppu;
	cpuWindow.cpu = &emulator.cpu;
	apuWindow.apu = &emulator.apu;
}

enum Domain {
	CpuRam,
	CpuBus,
	PpuBus,
	CIRam, // Nametables
	Oam,
	PrgRom,
	ChrRom
};

std::vector<MemoryDomain> Core::GetMemoryDomains() {
	return {
		{ CpuRam, "RAM", 0x800 },
		{ CpuBus, "Cpu Bus", 0x10000 },
		{ PpuBus, "Ppu Bus", 0x8000 },
		{ CIRam, "CIRam (Nametables)", 0x800 },
		{ Oam, "OAM", 0x100 },
		{ PrgRom, "Prg Rom", emulator.cartridge->prg.size() },
		{ ChrRom, "Chr Rom", emulator.cartridge->chr.size() },
	};
}

void Core::WriteMemory(int domain, size_t address, uint8_t val) {
	switch(domain) {
		case CpuRam:
			emulator.CpuRam[address] = val;
			break;
		case CpuBus:
		case PpuBus: break;
		case CIRam:
			emulator.ppu.vram[address] = val;
			break;
		case Oam:
			reinterpret_cast<uint8_t*>(emulator.ppu.oam)[address] = val;
			break;
		case PrgRom:
		case ChrRom: break;
	}
}
uint8_t Core::ReadMemory(int domain, size_t address) {
	switch(domain) {
		case CpuRam: return emulator.CpuRam[address];
		case CpuBus: return emulator.CpuRead(address, true);
		case PpuBus: return emulator.ppu.ppuRead(address, true);
		case CIRam: return emulator.ppu.vram[address];
		case Oam: return reinterpret_cast<uint8_t*>(emulator.ppu.oam)[address];
		case PrgRom: return emulator.cartridge->prg[address];
		case ChrRom: return emulator.cartridge->chr[address];
	}
	return 0;
}

void Core::Draw() {
	auto mapper = emulator.cartridge.get();

	if(auto nsf = dynamic_cast<NsfMapper*>(mapper)) {
		ImGui::Begin("Screen");

		ImGui::Text("title: %.32s\n", nsf->nsf.songName);
		ImGui::Text("artist: %.32s\n", nsf->nsf.artist);
		ImGui::Text("copyright: %.32s\n", nsf->nsf.copyright);

		ImGui::End();
	} else {
		DrawTextureWindow(this->texture, 8.0 / 7.0);
	}

	cpuWindow.DrawWindow();
	tables.DrawWindow();
	// tasEdit.DrawWindow(0);
	apuWindow.DrawWindow();
	disassembler.DrawWindow();
}

void Core::DrawMenuBar(bool& menuOpen) {
	if(ImGui::BeginMenu("NES")) {
		menuOpen = true;

		if(ImGui::MenuItem("Tas Editor", nullptr, false, false)) {
			// tasEdit.Open();
		}
		if(ImGui::MenuItem("CPU Viewer")) {
			cpuWindow.Open();
		}
		if(ImGui::MenuItem("PPU Viewer")) {
			tables.Open();
		}
		if(ImGui::MenuItem("APU Visuals")) {
			apuWindow.Open();
		}
		if(ImGui::MenuItem("Disassembler")) {
			disassembler.Open(emulator.cartridge->prg);
		}

		ImGui::EndMenu();
	}
}

void Core::SaveState(saver& saver) {
	emulator.SaveState(saver);
}

void Core::LoadState(saver& saver) {
	emulator.LoadState(saver);
}

void Core::LoadRom(const std::string& path) {
	auto ext = fs::path(path).extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });

	if(ext == ".nes") {
		std::shared_ptr<Mapper> cart;

		try {
			cart = LoadCart(path);
		} catch(std::exception& e) {
			logger.Log("Failed to load rom: %s\n", e.what());
			return;
		}

		// TODO: hard reset
		emulator.apu.vrc6 = false;
		emulator.InsertCartridge(cart);
		emulator.HardReset();

		currentFile = path;
	} else if(ext == ".nsf") {
		const auto mapper = std::make_shared<NsfMapper>(path);

		std::shared_ptr<Mapper> ptr = mapper;
		emulator.InsertCartridge(ptr);
		emulator.HardReset();

		emulator.controller1 = std::make_shared<StandardController>(0);
		emulator.controller2 = std::make_shared<StandardController>(1);

		emulator.ppu.Control.enableNMI = true;

		emulator.apu.vrc6 = mapper->nsf.extraSoundChip.vrc6;
		if(mapper->nsf.extraSoundChip.vrc7) logger.LogScreen("vrc7 not supported\n");
		if(mapper->nsf.extraSoundChip.fds) logger.LogScreen("FDS not supported\n");
		if(mapper->nsf.extraSoundChip.mmc5) logger.LogScreen("MMC5 not supported\n");
		if(mapper->nsf.extraSoundChip.namco163) logger.LogScreen("Namoc 163 not supported\n");
		if(mapper->nsf.extraSoundChip.sunsoft5B) logger.LogScreen("Sunsoft 5B not supported\n");

		apuWindow.Open();

		currentFile = path;
	} else if(ext == ".fm2") {
		/*if(!nes) {
			return;
		}

		runningTas = true;
		auto inputs = TasInputs::LoadFM2(path);
		nes->Reset();
		nes->controller1 = controller1 = std::make_shared<TasController>(inputs.Controller1);
		nes->controller2 = controller2 = std::make_shared<TasController>(inputs.Controller2);*/
	} else {
		logger.Log("Unknown file type");
	}
}

void Core::Reset() {
	emulator.Reset();
}

void Core::HardReset() {
	emulator.HardReset();
}

void Core::Update() {
	// 89342 cycles per frame
	while(!emulator.ppu.frameComplete) {
		emulator.Clock();
	}
	emulator.ppu.frameComplete = false;

	/*if(runningTas) {
		controller1->Frame();
		controller2->Frame();
	}*/

	// step = false;
	emulator.apu.lastBufferPos = emulator.apu.bufferPos;
	emulator.apu.bufferPos = 0;
}

}
