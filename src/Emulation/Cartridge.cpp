#include <fstream>
#include <cmath>

#include "Cartridge.h"
#include "Mappers/Mappers.h"
#include "../md5.h"
#include "logger.h"

struct INESheader {
	char name[4];
	uint8_t PrgRomSize;
	uint8_t ChrRomSize;

	uint8_t Flags6;

	uint8_t Flags7;
	uint8_t prgRamSize;
	uint8_t tvSystem1;
	uint8_t tvSystem2;

	uint8_t unused[5];
};

static bool isPowOf2(int n) {
	if(n == 0) return false;

	return ceil(log2(n)) == floor(log2(n));
}

std::shared_ptr<Mapper> LoadCart(const std::string& path) {
	logger.Log("Loading nes ROM: %s\n", path.c_str());
	
	std::ifstream stream(path, std::ios::binary);
	if(!stream.good()) {
		throw std::runtime_error("Could not open file");
	}
	
	INESheader header;
	stream.read((char*)&header, sizeof(header));

	if(!(header.name[0] == 'N' &&
		header.name[1] == 'E' &&
		header.name[2] == 'S' &&
		header.name[3] == 0x1A)) {
		throw std::invalid_argument("Invalid iNES header");
	}

	if(header.Flags6 & 0b0100) {
		// skip trainer
		stream.seekg(512, std::ios_base::cur);
	}

	uint8_t mapperId = (header.Flags6 >> 4) | (header.Flags7 & 0xF0);

	if((header.Flags7 & 0b1100) == 0b1000) {
		throw std::logic_error("iNES 2.0 not implemented");
	}

	int prgBanks = header.PrgRomSize;
	int chrBanks = header.ChrRomSize;
	logger.Log("Mapper:%i, PRG:%i, CHR:%i  \n", mapperId, prgBanks, chrBanks);

	if(prgBanks == 0) {
		throw std::runtime_error("Empty prg");
	}
	if(!isPowOf2(prgBanks)) {
		throw std::runtime_error("Prg not power of 2");
	}
	if(chrBanks != 0 && !isPowOf2(chrBanks)) {
		throw std::runtime_error("Chr not power of 2");
	}

	std::vector<uint8_t> prgRom;
	std::vector<uint8_t> chrRom;

	prgRom.resize(prgBanks * 0x4000);
	chrRom.resize(chrBanks * 0x2000);

	stream.read((char*)prgRom.data(), prgRom.size());
	stream.read((char*)chrRom.data(), chrRom.size());

	std::shared_ptr<Mapper> mapper;
	switch(mapperId) {
		case 0: mapper = std::make_shared<Mapper000>(prgRom, chrRom); break;
		case 1: mapper = std::make_shared<Mapper001>(prgRom, chrRom); break;
		case 2: mapper = std::make_shared<Mapper002>(prgRom, chrRom); break;
		case 3: mapper = std::make_shared<Mapper003>(prgRom, chrRom); break;
		case 4: mapper = std::make_shared<Mapper004>(prgRom, chrRom); break;
		case 7: mapper = std::make_shared<Mapper007>(prgRom, chrRom); break;
		// case 24: mapper = std::make_shared<VRC6Mapper>(prgRom, chrRom, false);
		// case 26: mapper = std::make_shared<VRC6Mapper>(prgRom, chrRom, true);
		case 65: mapper = std::make_shared<Mapper065>(prgRom, chrRom); break;
		default: throw std::logic_error("Mapper not implemented");
	}
	mapper->mirror = header.Flags6 & 1 ? MirrorMode::Vertical : MirrorMode::Horizontal;
	if(header.Flags6 & 8) {
		mapper->mirror = MirrorMode::FourScreen;
	}

	stream.clear();
	stream.seekg(0, std::ios::beg);
	mapper->hash = md5(stream);
	
	return mapper;
}
