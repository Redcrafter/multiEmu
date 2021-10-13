#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

class saver {
  private:
	std::vector<uint8_t> data;

  public:
	uint32_t readPos = 0;

	saver() = default;
	saver(const std::string& path);

	void Save(const std::string& path);

	template<typename T>
	void operator<<(const T& value) {
		// todo: make more efficient? 
		data.reserve(data.size() + sizeof(T));
		std::copy_n((uint8_t*)&value, sizeof(T), std::back_inserter(data));
	}

	template<typename T>
	void operator>>(T& value) {
		std::copy_n(this->data.begin() + readPos, sizeof(T), (uint8_t*)&value);
		readPos += sizeof(T);
	}
};
