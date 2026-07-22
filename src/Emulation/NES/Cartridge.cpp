#include "Cartridge.h"

#include <fstream>
#include <map>
#include <nlohmann/json.hpp>

#include "../../fs.h"
#include "../../logger.h"
#include "../../md5.h"
#include "../../sha1.h"

#include "Mappers/Mapper000.h"
#include "Mappers/Mapper001.h"
#include "Mappers/Mapper002.h"
#include "Mappers/Mapper003.h"
#include "Mappers/Mapper004.h"
// #include "Mappers/Mapper005.h"
// #include "Mappers/Mapper006.h"
#include "Mappers/Mapper007.h"
#include "Mappers/Mapper011.h"
#include "Mappers/Mapper065.h"
#include "Mappers/Mapper071.h"
#include "Mappers/Mapper079.h"
#include "Mappers/Mapper232.h"
#include "Mappers/VRC6Mapper.h"

namespace Nes {

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

struct dbItem {
	std::string name;
	uint16_t mapper;
	sha1 prgHash;
};

static bool dbInitialized = false;
static std::map<sha1, dbItem> cartDb;

static void InsertPrg(const std::string& name, uint16_t mapper, const std::string& hash_str) {
	auto hash = sha1::FromString(hash_str);
	if(cartDb.count(hash)) {
		auto& el = cartDb[hash];
		if(el.mapper != mapper) {
			// we only care about the mapper
			logger.Log("duplicate hash found: %s from %s\n", hash.ToString().c_str(), name.c_str());
		}
	} else {
		cartDb.insert(std::make_pair(hash, dbItem { name, mapper, hash }));
	}
}

static void InsertCart(const std::string& name, nlohmann::json& obj) {
	auto& board = obj["board"];
	auto mapper = (uint16_t)std::stoi(board["@mapper"].get<std::string>());
	auto& prg = board["prg"];

	if(prg.is_array()) {
		for(auto& entry : prg) {
			InsertPrg(name, (uint16_t)mapper, entry["@sha1"]);
		}
	} else {
		InsertPrg(name, mapper, prg["@sha1"]);
	}
}

void LoadCardDb(const std::string& path) {
	if(dbInitialized) {
		return;
	}
	dbInitialized = true;

	logger.Log("Loading nes cart db\n");

	try {
		std::ifstream f(path);
		auto test = nlohmann::json::parse(f);

		for(auto& game : test["database"]["game"]) {
			std::string name = game["@name"];

			auto& cartridge = game["cartridge"];
			if(cartridge.is_object()) {
				InsertCart(name, cartridge);
			} else if(cartridge.is_array()) {
				for(auto& obj : cartridge) {
					InsertCart(name, obj);
				}
			} else {
				logger.Log("invalid cart type\n");
			}
		}
		logger.Log("Finished loading %lu entries\n", cartDb.size());
	} catch(std::exception& e) {
		logger.Log("Failed to load cartDb: %s\n", e.what());
	}
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

	if(prgBanks == 0) {
		throw std::runtime_error("Empty prg");
	}

	std::vector<uint8_t> prgRom;
	std::vector<uint8_t> chrRom;

	prgRom.resize(prgBanks * 0x4000);
	chrRom.resize(chrBanks * 0x2000);

	stream.read((char*)prgRom.data(), prgRom.size());
	stream.read((char*)chrRom.data(), chrRom.size());

	sha1 prgHash((char*)prgRom.data(), prgRom.size());
	// sha1 chrHash((char*)chrRom.data(), chrRom.size());

	logger.Log("prg sha1: %s\n", prgHash.ToString().c_str());
	// logger.Log("chr sha1: %s\n", chrHash.ToString().c_str());
	if(cartDb.count(prgHash)) { // roms sometimes report wrong mapper id so we do a lookup for the rom hash
		const auto item = cartDb[prgHash];
		mapperId = item.mapper;

		logger.Log("Found cartridge \"%s\" in cartdb\n", item.name.c_str());
	} else {
		logger.Log("Couldn't find cartridge in cartdb\n");
	}

	logger.Log("Mapper:%i, PRG:%i, CHR:%i  \n", mapperId, prgBanks, chrBanks);
	std::shared_ptr<Mapper> mapper;
	switch(mapperId) {
		case 0: mapper = std::make_shared<Mapper000>(prgRom, chrRom); break;
		case 1: mapper = std::make_shared<Mapper001>(prgRom, chrRom); break;
		case 2: mapper = std::make_shared<Mapper002>(prgRom, chrRom); break;
		case 3: mapper = std::make_shared<Mapper003>(prgRom, chrRom); break;
		case 4: mapper = std::make_shared<Mapper004>(prgRom, chrRom); break;
		case 7: mapper = std::make_shared<Mapper007>(prgRom, chrRom); break;
		case 11: mapper = std::make_shared<Mapper011>(prgRom, chrRom); break;
		case 24: mapper = std::make_shared<VRC6Mapper>(prgRom, chrRom, false); break;
		case 26: mapper = std::make_shared<VRC6Mapper>(prgRom, chrRom, true); break;
		case 65: mapper = std::make_shared<Mapper065>(prgRom, chrRom); break;
		case 71: mapper = std::make_shared<Mapper071>(prgRom, chrRom); break;
		case 79: mapper = std::make_shared<Mapper079>(prgRom, chrRom); break;
		case 232: mapper = std::make_shared<Mapper232>(prgRom, chrRom); break;
		default: throw std::logic_error("Mapper not implemented");
	}
	if(header.Flags6 & 1) {
		mapper->mirror = mapper->defaultMirror = MirrorMode::Vertical;
	} else if(header.Flags6 & 8) {
		mapper->mirror = mapper->defaultMirror = MirrorMode::FourScreen;
	} else {
		mapper->mirror = mapper->defaultMirror = MirrorMode::Horizontal;
	}

	stream.clear();
	stream.seekg(0, std::ios::beg);
	mapper->hash = md5(stream);
	auto hasSram = (header.Flags6 >> 1) & 1;

	if(hasSram) {
		auto path = "./saves/NES/" + mapper->hash.ToString() + ".saveRam";
		mapper->MapSaveRam(path);
	}

	return mapper;
}

}