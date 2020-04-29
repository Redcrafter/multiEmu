#include "Cartridge.h"
#include <fstream>
#include "Mappers/Mappers.h"

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

Cartridge::Cartridge(std::string path) {
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
		throw std::invalid_argument("Inavlid iNES header");
	}

	if(header.Flags6 & 0b0100) {
		stream.seekg(512, std::ios_base::cur);
	}

	mapperId = (header.Flags6 >> 4) | (header.Flags7 & 0xF0);

	if((header.Flags7 & 0b1100) == 0b1000) {
		throw std::logic_error("Not implemented"); // NES 2.0
	}

	prgBanks = header.PrgRomSize;
	chrBanks = header.ChrRomSize;

	prgRom.resize(0x4000 * prgBanks);
	chrRom.resize(0x2000 * chrBanks);

	stream.read((char*)prgRom.data(), prgRom.size());
	stream.read((char*)chrRom.data(), chrRom.size());

	printf("%i, PRG:%i, CHR:%i  \n", mapperId, prgBanks, chrBanks);
	switch(mapperId) {
		case 0: mapper = std::make_shared<Mapper000>(prgBanks, chrBanks); break;
		case 1: mapper = std::make_shared<Mapper001>(prgBanks, chrBanks); break;
		case 2: mapper = std::make_shared<Mapper002>(prgBanks, chrBanks); break;
		case 3: mapper = std::make_shared<Mapper003>(prgBanks, chrBanks); break;
		case 4: mapper = std::make_shared<Mapper004>(prgBanks, chrBanks); break;
		case 7: mapper = std::make_shared<Mapper007>(prgBanks, chrBanks); break;
		default: throw std::logic_error("Mapper not implemented");
	}
	mapper->mirror = header.Flags6 & 1 ? MirrorMode::Vertical : MirrorMode::Horizontal;

	stream.clear();
	stream.seekg(0, std::ios::beg);
	hash = new md5(stream);
}

Cartridge::~Cartridge() {
	delete hash;
}

bool Cartridge::cpuRead(uint16_t addr, uint8_t& data) {
	uint32_t mapped = 0;
	int res = mapper->cpuMapRead(addr, mapped);

	switch(res) {
		case 1:
			data = prgRom[mapped & (prgBanks * 0x4000 - 1)];
			return true;
		case 2:
			data = mapped;
			return true;
	}

	return false;
}

bool Cartridge::cpuWrite(uint16_t addr, uint8_t data) {
	return mapper->cpuMapWrite(addr, data);
}

bool Cartridge::ppuRead(uint16_t addr, uint8_t& data, bool readOnly) {
	uint32_t mapped = 0;
	if(mapper->ppuMapRead(addr, mapped, readOnly) && chrBanks != 0) {
		data = chrRom[mapped & (chrBanks * 0x2000 - 1)];
		return true;
	}
	return false;
}

bool Cartridge::ppuWrite(uint16_t addr, uint8_t data) {
	return mapper->ppuMapWrite(addr, data);
}
