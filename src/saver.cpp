#include "saver.h"
#include <fstream>

saver::saver(std::string path) {
	std::ifstream stream(path, std::ios::binary);
	uint64_t size = 0;
	stream.read(reinterpret_cast<char*>(&size), 8);

	if(size >= 1024 * 1024) {
		throw std::runtime_error("Save file too big"); // Prevent loading files > 1 Mb
	}
	data.resize(size);
	stream.read(reinterpret_cast<char*>(data.data()), size);
}

void saver::Save(std::string path) {
	std::ofstream stream(path, std::ios::binary);

	auto size = data.size();
	stream.write(reinterpret_cast<char*>(&size), 8);
	stream.write(reinterpret_cast<char*>(data.data()), size);

	stream.close();
}

void saver::operator<<(bool val) {
	Write(&val, 1);
}

void saver::operator<<(uint8_t val) {
	Write(&val, 1);
}

void saver::operator<<(uint16_t val) {
	Write(&val, 2);
}

void saver::operator<<(int32_t val) {
	Write(&val, 4);
}

void saver::operator<<(uint32_t val) {
	Write(&val, 4);
}

void saver::operator<<(uint64_t val) {
	Write(&val, 8);
}

void saver::operator>>(bool& val) {
	val = data[readPos];
	readPos++;
}

void saver::operator>>(uint8_t& val) {
	val = data[readPos];
	readPos++;
}

void saver::operator>>(uint16_t& val) {
	val = *reinterpret_cast<uint16_t*>(&data[readPos]);
	readPos += 2;
}

void saver::operator>>(int32_t& val) {
	val = *reinterpret_cast<uint16_t*>(&data[readPos]);
	readPos += 4;
}

void saver::operator>>(uint32_t& val) {
	val = *reinterpret_cast<uint16_t*>(&data[readPos]);
	readPos += 4;
}

void saver::operator>>(uint64_t& val) {
	val = *reinterpret_cast<uint16_t*>(&data[readPos]);
	readPos += 8;
}
