#include "saver.h"
#include <fstream>

saver::saver() {
}

saver::~saver() {
}

saver::saver(std::string path) {
	std::ifstream stream(path, std::ios::binary);
	uint64_t size = 0;
	stream.read(reinterpret_cast<char*>(&size), 8);

	if(size >= 1024 * 1024) {
		throw new std::runtime_error("Save file too big"); // Prevent loading files > 1 Mb
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

void saver::Write(const char* data, int size) {
	for(int i = 0; i < size; ++i) {
		this->data.push_back(data[i]);
	}
}

void saver::Read(char* data, int size) {
	for(int i = 0; i < size; ++i) {
		data[i] = this->data[readPos];
		readPos++;
	}
}

char* saver::ReadString() {
	uint64_t size;
	(*this) >> size;

	char* str = new char[size];
	Read(str, size);

	return str;
}

void saver::operator<<(bool val) {
	Write(reinterpret_cast<char*>(&val), 1);
}

void saver::operator<<(uint8_t val) {
	Write(reinterpret_cast<char*>(&val), 1);
}

void saver::operator<<(uint16_t val) {
	Write(reinterpret_cast<char*>(&val), 2);
}

void saver::operator<<(int32_t val) {
	Write(reinterpret_cast<char*>(&val), 4);
}

void saver::operator<<(uint32_t val) {
	Write(reinterpret_cast<char*>(&val), 4);
}

void saver::operator<<(uint64_t val) {
	Write(reinterpret_cast<char*>(&val), 8);
}

void saver::operator<<(std::string str) {
	(*this) << str.length();
	Write(str.c_str(), str.length());
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
