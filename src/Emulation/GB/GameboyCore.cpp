#include "GameboyCore.h"

#include <fstream>
#include <vector>

#include "../../Input.h"

#include "Mappers/MBC1.h"
#include "Mappers/NoMBC.h"

namespace Gameboy {

static const uint8_t logo[] = {
	0xCE, 0xED, 0x66, 0x66, 0xCC, 0x0D, 0x00, 0x0B, 0x03, 0x73, 0x00, 0x83, 0x00, 0x0C, 0x00, 0x0D,
	0x00, 0x08, 0x11, 0x1F, 0x88, 0x89, 0x00, 0x0E, 0xDC, 0xCC, 0x6E, 0xE6, 0xDD, 0xDD, 0xD9, 0x99,
	0xBB, 0xBB, 0x67, 0x63, 0x6E, 0x0E, 0xEC, 0xCC, 0xDD, 0xDC, 0x99, 0x9F, 0xBB, 0xB9, 0x33, 0x3E
};

static std::vector<uint8_t> readFile(const std::string& path) {
	std::ifstream input(path, std::ios::binary);
	return std::vector<uint8_t>(std::istreambuf_iterator<char>(input), {});
}

enum Domain {
	CpuRam,
	Hram,
	CpuBus,
	VRam,
	Oam,
	Rom
};

GameboyColorCore::GameboyColorCore() : texture(160, 144), gameboy(texture) {
	Input::SetMapper(Input::InputMapper("GB", {
		{ "Right",  0, { { GLFW_KEY_RIGHT, 0 } } },
		{ "Left",   1, { { GLFW_KEY_LEFT,  0 } } },
		{ "Up",     2, { { GLFW_KEY_UP,    0 } } },
		{ "Down",   3, { { GLFW_KEY_DOWN,  0 } } },
		{ "A",      4, { { GLFW_KEY_A,     0 } } },
		{ "B",      5, { { GLFW_KEY_B,     0 } } },
		{ "Select", 6, { { GLFW_KEY_ENTER, 0 } } },
		{ "Start",  7, { { GLFW_KEY_S,     0 } } },
	}));
}

std::vector<MemoryDomain> GameboyColorCore::GetMemoryDomains() {
	return {
		{ CpuRam, "RAM", sizeof(gameboy.ram) },
		{ Hram, "HRAM", sizeof(gameboy.hram) },
		{ CpuBus, "Cpu Bus", 0x10000 },
		{ VRam, "VRAM", sizeof(gameboy.ppu.VRAM) },
		{ Oam, "OAM", sizeof(gameboy.ppu.OAM) },
		{ Rom, "Rom", gameboy.mbc->rom.size() }
	};
}

void GameboyColorCore::WriteMemory(int domain, size_t address, uint8_t val) {
	switch(domain) {
		case CpuRam: ((uint8_t*)&gameboy.ram)[address] = val; break;
		case Hram: gameboy.hram[address] = val; break;
		case CpuBus: gameboy.CpuWrite(address, val); break;
		case VRam: ((uint8_t*)&gameboy.ppu.VRAM)[address] = val; break;
		case Oam: gameboy.ppu.OAM[address] = val; break;
		case Rom: gameboy.mbc->rom[address] = val; break;
	}
}

uint8_t GameboyColorCore::ReadMemory(int domain, size_t address) {
	switch(domain) {
		case CpuRam: return ((uint8_t*)&gameboy.ram)[address];
		case Hram: return gameboy.hram[address];
		case CpuBus: return gameboy.CpuRead(address);
		case VRam: return ((uint8_t*)&gameboy.ppu.VRAM)[address];
		case Oam: return gameboy.ppu.OAM[address];
		case Rom: return gameboy.mbc->rom[address];
	}

	throw std::logic_error("Unreachable");
}

void GameboyColorCore::LoadRom(const std::string& path) {
	auto data = readFile(path);

	for(size_t i = 0; i < sizeof(logo); i++) {
		if(data[0x0104 + i] != logo[i]) {
			throw std::runtime_error("Not a gameboy game");
		}
	}

	/*uint8_t cgbFlag = data[0x143];
	if(cgbFlag == 0x80) {
        // Game supports CGB functions, but works on old gameboys also.
        gameboy.gbc = true;
    } else if(cgbFlag == 0xC0) {
        // Game works on CGB only (physically the same as 80h).
        gameboy.gbc = true;
    } else {
        gameboy.gbc = false;
    }*/
	gameboy.gbc = false;

	auto sgbFlag = data[0x0146];
	if(sgbFlag == 0x03) {
		// Game supports SGB functions
	}
	uint8_t cartType = data[0x0147];
	uint32_t romSize = data[0x0148];
	if(romSize < 8) {
		romSize = 0x8000 << romSize;
	} else {
		throw std::runtime_error("Invalid rom size");
	}

	if(data.size() != romSize) {
		throw std::runtime_error("File size not equal to rom size");
	}

	uint32_t ramSize = data[0x0149];
	switch(ramSize) {
		case 0: ramSize = 0x0; break;
		case 1: ramSize = 0x800; break;
		case 2: ramSize = 0x2000; break;
		case 3: ramSize = 0x8000; break;
		default: throw std::runtime_error("Invalid ram size");
	}

	uint8_t headerChecksum = data[0x014D];
	uint8_t x = 0;
	for(int i = 0x0134; i <= 0x014C; i++) {
		x = x - data[i] - 1;
	}
	if(headerChecksum != x) {
		throw std::runtime_error("Invalid header checksum");
	}

	uint16_t globalChecksum = data[0x014E] << 8 | data[0x014F];
	uint32_t check = 0;
	for(uint8_t i : data) {
		check += i;
	}
	check -= globalChecksum & 0xFF;
	check -= globalChecksum >> 8;
	if(globalChecksum != (check & 0xFFFF)) {
		// throw std::runtime_error("Invalid global checksum");
	}

	romHash = md5((char*)data.data(), data.size());

	switch(cartType) {
		case 0:
			assert(ramSize == 0);
			gameboy.mbc = std::make_unique<NoMBC>(data, ramSize);
			gameboy.mbc->hasRam = false;
			gameboy.mbc->hasBattery = false;
			break;
		case 1:
			assert(ramSize == 0);
			gameboy.mbc = std::make_unique<MBC1>(data, ramSize);
			gameboy.mbc->hasRam = false;
			gameboy.mbc->hasBattery = false;
			break;
		case 2:
			gameboy.mbc = std::make_unique<MBC1>(data, ramSize);
			gameboy.mbc->hasRam = ramSize != 0;
			gameboy.mbc->hasBattery = false;
			break;
		case 3:
			assert(ramSize != 0);
			gameboy.mbc = std::make_unique<MBC1>(data, ramSize);
			gameboy.mbc->hasRam = true;
			gameboy.mbc->hasBattery = true;
			break;
		case 8:
			assert(ramSize != 0);
			gameboy.mbc = std::make_unique<NoMBC>(data, ramSize);
			gameboy.mbc->hasRam = true;
			gameboy.mbc->hasBattery = false;
			break;
		case 9:
			assert(ramSize != 0);
			gameboy.mbc = std::make_unique<NoMBC>(data, ramSize);
			gameboy.mbc->hasRam = true;
			gameboy.mbc->hasBattery = true;
			break;
	}

	gameboy.Reset();
}

void GameboyColorCore::Reset() {
	gameboy.Reset();
}

void GameboyColorCore::Update() {
	while(!gameboy.ppu.frameComplete) {
		gameboy.Clock();
	}
	gameboy.ppu.frameComplete = false;
}

}
