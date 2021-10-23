#include "GameboyCore.h"

#include <fstream>
#include <vector>

#include "../../Input.h"
#include "../../fs.h"

#include "Mappers/GbsMBC.h"
#include "Mappers/MBC1.h"
#include "Mappers/MBC2.h"
#include "Mappers/MBC3.h"
#include "Mappers/MBC5.h"
#include "Mappers/NoMBC.h"

namespace Gameboy {

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

Core::Core() : texture(160, 144), gameboy(texture), _ppuWindow(gameboy.ppu) { }

std::vector<MemoryDomain> Core::GetMemoryDomains() {
	return {
		{ CpuRam, "RAM", sizeof(gameboy.ram) },
		{ Hram, "HRAM", sizeof(gameboy.hram) },
		{ CpuBus, "Cpu Bus", 0x10000 },
		{ VRam, "VRAM", sizeof(gameboy.ppu.VRAM) },
		{ Oam, "OAM", sizeof(gameboy.ppu.OAM) },
		{ Rom, "Rom", gameboy.mbc->rom.size() }
	};
}

void Core::WriteMemory(int domain, size_t address, uint8_t val) {
	switch(domain) {
		case CpuRam: ((uint8_t*)&gameboy.ram)[address] = val; break;
		case Hram: gameboy.hram[address] = val; break;
		case CpuBus: gameboy.CpuWrite(address, val); break;
		case VRam: ((uint8_t*)&gameboy.ppu.VRAM)[address] = val; break;
		case Oam: gameboy.ppu.OAM[address] = val; break;
		case Rom: gameboy.mbc->rom[address] = val; break;
	}
}

uint8_t Core::ReadMemory(int domain, size_t address) {
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

void Core::LoadRom(const std::string& path) {
	auto ext = fs::path(path).extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });

	if(ext == ".gb" || ext == ".gbc") {
		auto data = readFile(path);

		if(!checkLogo(&data[0x0104])) {
			throw std::runtime_error("Not a gameboy game");
		}

		mode = Mode::DMG;
		uint8_t cgbFlag = data[0x143];
		if(cgbFlag == 0x80 || cgbFlag == 0xC0) {
			mode = Mode::CGB;
		}
		auto sgbFlag = data[0x0146];
		if(sgbFlag == 0x03) {
			mode = Mode::SGB;
		}
		uint8_t cartType = data[0x0147];
		uint32_t romSize = data[0x0148];
		if(romSize < 9) {
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
		for(const uint8_t i : data) check += i;
		check -= globalChecksum & 0xFF;
		check -= globalChecksum >> 8;
		if(globalChecksum != (check & 0xFFFF)) {
			// throw std::runtime_error("Invalid global checksum");
		}

		romHash = md5((char*)data.data(), data.size());

		switch(cartType) {
			case 0x0:  assert(ramSize == 0); gameboy.mbc = std::make_unique<NoMBC>(data, ramSize, false); break;
			case 0x1:  assert(ramSize == 0); gameboy.mbc = std::make_unique<MBC1>(data, ramSize, false); break;
			case 0x2: 						 gameboy.mbc = std::make_unique<MBC1>(data, ramSize, false); break;
			case 0x3:  assert(ramSize != 0); gameboy.mbc = std::make_unique<MBC1>(data, ramSize, true); break;
			case 0x5:  assert(ramSize == 0); gameboy.mbc = std::make_unique<MBC2>(data, false); break;
			case 0x6:  assert(ramSize == 0); gameboy.mbc = std::make_unique<MBC2>(data, true); break;
			case 0x8:  assert(ramSize != 0); gameboy.mbc = std::make_unique<NoMBC>(data, ramSize, false); break;
			case 0x9:  assert(ramSize != 0); gameboy.mbc = std::make_unique<NoMBC>(data, ramSize, true); break;
			case 0x0F: assert(ramSize == 0); gameboy.mbc = std::make_unique<MBC3>(data, ramSize, true, true); break;
			case 0x10: assert(ramSize != 0); gameboy.mbc = std::make_unique<MBC3>(data, ramSize, true, true); break;
			case 0x11: assert(ramSize == 0); gameboy.mbc = std::make_unique<MBC3>(data, ramSize, false, false); break;
			case 0x12: assert(ramSize != 0); gameboy.mbc = std::make_unique<MBC3>(data, ramSize, false, false); break;
			case 0x13: assert(ramSize != 0); gameboy.mbc = std::make_unique<MBC3>(data, ramSize, true, false); break;
			case 0x19: assert(ramSize == 0); gameboy.mbc = std::make_unique<MBC5>(data, ramSize, false, false); break;
			case 0x1A: assert(ramSize != 0); gameboy.mbc = std::make_unique<MBC5>(data, ramSize, false, false); break;
			case 0x1B: assert(ramSize != 0); gameboy.mbc = std::make_unique<MBC5>(data, ramSize, true, false); break;
			case 0x1C: assert(ramSize == 0); gameboy.mbc = std::make_unique<MBC5>(data, ramSize, false, true); break;
			case 0x1D: assert(ramSize != 0); gameboy.mbc = std::make_unique<MBC5>(data, ramSize, false, true); break;
			case 0x1E: assert(ramSize != 0); gameboy.mbc = std::make_unique<MBC5>(data, ramSize, true, true); break;
			default: throw std::runtime_error("unknown mbc " + std::to_string(cartType));
		}
		
		gameboy.Reset(mode);
	} else if(ext == ".gbs") {
		gameboy.mbc = std::make_unique<GbsMBC>(gameboy, path);
		((GbsMBC*)gameboy.mbc.get())->LoadTrack();
	}
}

void Core::Reset() {
	gameboy.Reset(mode);
}

void Core::Update() {
	const auto cycles = 4194304 / 60.0;

	while(gameboy.cyclesPassed < cycles) {
		gameboy.Clock();
	}
	gameboy.cyclesPassed -= cycles;

	if(mode == Mode::DMG && gameboy.cpu.state == CpuState::Stop) {
		texture.Clear({ 0xFF, 0xFF, 0xFF });
	}
}

void Core::Draw() {
	auto mbc = gameboy.mbc.get();

	if(auto gbs = dynamic_cast<GbsMBC*>(mbc)) {
		ImGui::Begin("Screen");

		if(currentTrack == -1) {
			currentTrack = selectedTrack = gbs->header.firstSong;
		}
		ImGui::Text("title: %.32s\n", gbs->header.title);
		ImGui::Text("author: %.32s\n", gbs->header.author);
		ImGui::Text("copyright: %.32s\n", gbs->header.copyright);
		ImGui::Text("track: %i/%i\n", currentTrack, gbs->header.numSongs);

		ImGui::InputInt("##stuff", &selectedTrack);
		ImGui::SameLine();

		selectedTrack = std::max(selectedTrack, 1);
		selectedTrack = std::min(selectedTrack, (int)gbs->header.numSongs);
		if(ImGui::Button("Play")) {
			currentTrack = selectedTrack;
			gbs->LoadTrack(selectedTrack);
		}

		ImGui::End();
	} else {
		DrawTextureWindow(texture);
	}

	_ppuWindow.DrawWindow();
}

void Core::DrawMenuBar(bool& menuOpen) {
	if(ImGui::BeginMenu("GB")) {
		menuOpen = true;

		if(ImGui::MenuItem("PPU Viewer")) {
			_ppuWindow.Open();
		}

		ImGui::EndMenu();
	}
}

}
