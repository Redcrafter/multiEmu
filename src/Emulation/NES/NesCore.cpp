#include "NesCore.h"
#include "logger.h"
#include "Input.h"

#include <algorithm>

#include "Mappers/NsfMapper.h"
#include "StandardController.h"
#include "audio.h"

NesCore::NesCore(): texture(256, 240) {
	LoadCardDb("./NesCarts (2017-08-21).json");

	Input::SetMapper(InputMapper("Nes", {
		{ "Controller1 A", 0, Key {{ GLFW_KEY_A, 0 }} },
		{ "Controller1 B", 1, Key {{ GLFW_KEY_B, 0 }} },
		{ "Controller1 Start", 2, Key {{ GLFW_KEY_S, 0 }} },
		{ "Controller1 Select", 3, Key {{ GLFW_KEY_ENTER, 0 }} },
		{ "Controller1 Up", 4, Key {{ GLFW_KEY_UP, 0 }} },
		{ "Controller1 Down", 5, Key {{ GLFW_KEY_DOWN, 0 }} },
		{ "Controller1 Left", 6, Key {{ GLFW_KEY_LEFT, 0 }} },
		{ "Controller1 Right", 7, Key {{ GLFW_KEY_RIGHT, 0 }} },

		{ "Controller2 A", 8, Key {{ 0, 0 }} },
		{ "Controller2 B", 9, Key {{ 0, 0 }} },
		{ "Controller2 Start", 10, Key {{ 0, 0 }} },
		{ "Controller2 Select", 11, Key {{ 0, 0 }} },
		{ "Controller2 Up", 12, Key {{ 0, 0 }} },
		{ "Controller2 Down", 13, Key {{ 0, 0 }} },
		{ "Controller2 Left", 14, Key {{ 0, 0 }} },
		{ "Controller2 Right", 15, Key {{ 0, 0 }} },
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

std::vector<MemoryDomain> NesCore::GetMemoryDomains() {
	return {
		MemoryDomain { CpuRam, "RAM", 0x800 },
		MemoryDomain { CpuBus, "Cpu Bus", 0x10000 },
		MemoryDomain { PpuBus, "Ppu Bus", 0x8000 },
		MemoryDomain { CIRam, "CIRam (Nametables)", 0x800 },
		MemoryDomain { Oam, "OAM", 0x100 },
		MemoryDomain { PrgRom, "Prg Rom", emulator.cartridge->prg.size() },
		MemoryDomain { ChrRom, "Chr Rom", emulator.cartridge->chr.size() },
	};
}

void NesCore::WriteMemory(int domain, size_t address, uint8_t val) {
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
			emulator.ppu.pOAM[address] = val;
			break;
		case PrgRom:
		case ChrRom: break;
	}
}
uint8_t NesCore::ReadMemory(int domain, size_t address) {
	switch(domain) {
		case CpuRam: return emulator.CpuRam[address];
		case CpuBus: return emulator.CpuRead(address, true);
		case PpuBus: return emulator.ppu.ppuRead(address, true);
		case CIRam: return emulator.ppu.vram[address];
		case Oam: return emulator.ppu.pOAM[address];
		case PrgRom: return emulator.cartridge->prg[address];
		case ChrRom: return emulator.cartridge->chr[address];
	}
	return 0;
}

void NesCore::DrawMenuBar(bool& menuOpen) {
	if(ImGui::BeginMenu("NES")) {
		menuOpen = true;

		if(ImGui::MenuItem("Tas Editor", nullptr, false, false)) {
			// tasEdit.Open();
		}
		if(ImGui::MenuItem("CPU Viewer", nullptr, false)) {
			cpuWindow.Open();
		}
		if(ImGui::MenuItem("PPU Viewer", nullptr, false)) {
			tables.Open();
		}
		if(ImGui::MenuItem("APU Visuals", nullptr, false)) {
			apuWindow.Open();
		}

		ImGui::EndMenu();
	}
}

void NesCore::DrawWindows() {
	cpuWindow.DrawWindow();
	tables.DrawWindow();
	// tasEdit.DrawWindow(0);
	apuWindow.DrawWindow();
}

void NesCore::SaveState(saver& saver) {
	emulator.LoadState(saver);
}

void NesCore::LoadState(saver& saver) {
	emulator.SaveState(saver);
}

void NesCore::LoadRom(const std::string& path) {
    const auto asdf = fs::path(path);
    auto ext = asdf.extension().string();
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

void NesCore::Reset() {
	emulator.Reset();
}

void NesCore::HardReset() {
	emulator.HardReset();
}

void NesCore::Update() {
    // if(!nes || !running && !step) return;

	int mapped = 1;
	/*if(speedUp) {
		mapped = 5;
	}*/

	for(int i = 0; i < mapped; ++i) {
		// 89342 cycles per frame 
		while(!emulator.ppu.frameComplete) {
			emulator.Clock();
		}
		emulator.ppu.frameComplete = false;

		/*if(runningTas) {
			controller1->Frame();
			controller2->Frame();
		}*/
	}

	// step = false;
	emulator.apu.lastBufferPos = emulator.apu.bufferPos;
	emulator.apu.bufferPos = 0;
}