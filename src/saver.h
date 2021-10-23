#pragma once
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>
#include <vector>
#include <iterator>

class saver {
  private:
	std::vector<uint8_t> data;
	uint32_t readPos = 0;
	bool reading = false;

  public:
	saver() = default;
	saver(const std::string& path);

	void Save(const std::string& path);

	void write(const uint8_t* ptr, size_t size) {
		assert(!reading);
		data.reserve(data.size() + size);
		std::copy_n(ptr, size, std::back_inserter(data));
	}
	void read(uint8_t* ptr, size_t size) {
		assert(reading);
		assert(readPos + size <= data.size());
		std::copy_n(this->data.begin() + readPos, size, ptr);
		readPos += size;
	}

	template<typename T>
	saver& operator<<(const T& value) {
		assert(!reading);
		// todo: make more efficient?
		data.reserve(data.size() + sizeof(T));
		std::copy_n((uint8_t*)&value, sizeof(T), std::back_inserter(data));
		return *this;
	}

	template<typename T>
	saver& operator>>(T& value) {
		assert(reading);
		assert(readPos + sizeof(T) <= data.size());
		std::copy_n(this->data.begin() + readPos, sizeof(T), (uint8_t*)&value);
		readPos += sizeof(T);
		return *this;
	}

	void beginRead() {
		readPos = 0;
		reading = true;
	}
	void endRead() {
		assert(readPos == data.size());
		reading = false;
	}

	void clear() {
		data.clear();
	}
};
