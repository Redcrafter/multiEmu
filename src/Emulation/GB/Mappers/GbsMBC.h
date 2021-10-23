#pragma once
#include <fstream>
#include <string>

#include "../../../math.h"
#include "../Gameboy.h"
#include "MBC.h"

namespace Gameboy {

template<typename T>
void read(std::ifstream& stream, T& ref) {
	stream.read((char*)&ref, sizeof(T));
};

struct GbsHeader {
	char identifier[3];
	uint8_t version;
	uint8_t numSongs;
	uint8_t firstSong;
	uint16_t loadAddress;
	uint16_t initAddress;
	uint16_t playAddress;
	uint16_t SP;
	uint8_t TMA;
	uint8_t TAC;

	char title[32];
	char author[32];
	char copyright[32];
};
static_assert(sizeof(GbsHeader) == 0x70);

class GbsMBC : public MBC {
  private:
	Gameboy& gb;
	uint32_t romBank = 0x4000;

  public:
	GbsHeader header;

	GbsMBC(Gameboy& gb, const std::string& path);
	~GbsMBC() {}

	void LoadTrack(int track = -1);

	uint8_t Read0(uint16_t addr) const override { return rom[addr & 0x3FFF]; }
	uint8_t Read4(uint16_t addr) const override { return rom[romBank | (addr & 0x3FFF)]; }
	uint8_t ReadA(uint16_t addr) const override { return ram[addr & 0x1FFF]; }

	void Write0(uint16_t addr, uint8_t val) override {
		romBank = (val * 0x4000) & romMask;
	}
	void Write4(uint16_t addr, uint8_t val) override {}
	void WriteA(uint16_t addr, uint8_t val) override { ram[addr & 0x1FFF] = val; }

	void SaveState(saver& saver) override {}
	void LoadState(saver& saver) override {}
};

GbsMBC::GbsMBC(Gameboy& gb, const std::string& path) : gb(gb) {
	std::ifstream stream(path, std::ios::binary);

	read(stream, header);
	if(header.identifier[0] != 'G' ||
	   header.identifier[1] != 'B' ||
	   header.identifier[2] != 'S') {
		throw std::runtime_error("Invalid gbs format");
	}
	assert(header.loadAddress >= 0x400);

	auto data = std::vector<uint8_t>(std::istreambuf_iterator<char>(stream), {});

	auto romSize = math::roundPow2((data.size() + header.loadAddress + 0x3FFF) & ~0x3FFF);

	rom.resize(romSize);
	ram.resize(0x2000);
	romMask = romSize - 1;

	std::copy(data.begin(), data.end(), rom.begin() + header.loadAddress);

	bool hasInterrupts = header.TAC & 0x40;
	// Generate interrupt handlers
	for(unsigned i = 0; i <= (hasInterrupts ? 0x50 : 0x38); i += 8) {
		rom[i] = 0xc3; // jp $XXXX
		rom[i + 1] = (header.loadAddress + i);
		rom[i + 2] = (header.loadAddress + i) >> 8;
	}
	for(unsigned i = hasInterrupts ? 0x58 : 0x40; i <= 0x60; i += 8) {
		rom[i] = 0xc9; // ret
	}

	// Generate entry
	// generate_gbs_entry(gb, gb.rom + GBS_ENTRY);
	auto pos = 0x100;
	rom[pos++] = 0x31; // LD SP,d16
	rom[pos++] = header.SP;
	rom[pos++] = header.SP >> 8;
	rom[pos++] = 0x3E; // LD A,d8
	rom[pos++] = header.firstSong;
	rom[pos++] = 0xCD; // Call a16
	rom[pos++] = header.initAddress;
	rom[pos++] = header.initAddress >> 8;
	rom[pos++] = 0x76; // HALT
	rom[pos++] = 0x00; // NOP
	rom[pos++] = 0xAF; // XOR a
	rom[pos++] = 0xE0; // LDH [$FFXX], a
	rom[pos++] = 0x0F;
	rom[pos++] = 0xCD; // Call a16
	rom[pos++] = header.playAddress;
	rom[pos++] = header.playAddress >> 8;
	rom[pos++] = 0x18; // JR pc ± $XX
	rom[pos++] = -10;  // To HALT
}

void GbsMBC::LoadTrack(int track) {
	if(track == -1) {
		track = header.firstSong;
	}
	gb.Reset(gb.gbc ? Mode::CGB : Mode::DMG);

	gb.CpuWrite(0xFF40, 0x80);
	gb.CpuWrite(0xFF07, header.TAC);
	gb.CpuWrite(0xFF06, header.TMA);
	gb.CpuWrite(0xFF26, 0x80);
	gb.CpuWrite(0xFF25, 0xFF);
	gb.CpuWrite(0xFF24, 0x77);
	gb.CpuWrite(0xFFFF, header.TAC || header.TMA ? 4 : 1);

	rom[0x104] = track; // replace load track number
}

} // namespace Gameboy
