#pragma once
#include <cstdint>
#include <string>
#include <vector>

class saver {
private:
	std::vector<uint8_t> data;
public:
	uint32_t readPos = 0;

	saver() {}
	saver(std::string path);

	void Save(std::string path);

	template <typename T>
	void Write(T* data, size_t size) {
		char* ptr = (char*)data;
		for(size_t i = 0; i < size * sizeof(T); i++) {
			this->data.push_back(ptr[i]);
		}
	}
	template <typename T>
	void Read(T* data, size_t size) {
		char* ptr = (char*)data;
		for(size_t i = 0; i < size * sizeof(T); ++i) {
			ptr[i] = this->data[readPos];
			readPos++;
		}
	}

	void operator <<(bool val);
	void operator <<(uint8_t val);
	void operator <<(uint16_t val);
	void operator <<(int32_t val);
	void operator <<(uint32_t val);
	void operator <<(uint64_t val);

	void operator >>(bool& val);
	void operator >>(uint8_t& val);
	void operator >>(uint16_t& val);
	void operator >>(int32_t& val);
	void operator >>(uint32_t& val);
	void operator >>(uint64_t& val);
};
