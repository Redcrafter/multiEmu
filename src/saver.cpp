#include "saver.h"

#include <fstream>

#include "fs.h"

saver::saver(const std::string& path) {
	const auto size = fs::file_size(path);
	std::ifstream stream(path, std::ios::binary);

	if(size >= 64 * 1024 * 1024) {
		throw std::runtime_error("Save file too big"); // Prevent loading files > 64 Mb
	}
	data.resize(size);
	stream.read(reinterpret_cast<char*>(data.data()), size);
}

void saver::Save(const std::string& path) {
	std::ofstream stream(path, std::ios::binary);

	stream.write(reinterpret_cast<char*>(data.data()), data.size());

	stream.close();
}

void saver::operator<<(bool val) {
	Write(&val, 1);
}

void saver::operator<<(uint8_t val) {
	Write(&val, 1);
}

void saver::operator<<(uint16_t val) {
	Write(&val, 1);
}

void saver::operator<<(int32_t val) {
	Write(&val, 1);
}

void saver::operator<<(uint32_t val) {
	Write(&val, 1);
}

void saver::operator<<(uint64_t val) {
	Write(&val, 1);
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
	val = *reinterpret_cast<int32_t*>(&data[readPos]);
	readPos += 4;
}

void saver::operator>>(uint32_t& val) {
	val = *reinterpret_cast<uint32_t*>(&data[readPos]);
	readPos += 4;
}

void saver::operator>>(uint64_t& val) {
	val = *reinterpret_cast<uint64_t*>(&data[readPos]);
	readPos += 8;
}
