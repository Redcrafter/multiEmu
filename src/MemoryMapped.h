#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

class MemoryMapped {
  private:
	uint8_t* map = nullptr;
	size_t size = 0;

#ifdef _MSC_VER
	void* file = nullptr;
	void* mappedFile = nullptr;
#endif

  public:
	MemoryMapped(const std::string& file, size_t size);
	MemoryMapped(const MemoryMapped&) = delete;
	~MemoryMapped();

	size_t Size() { return size; }

	// uint8_t operator[](size_t pos);
	uint8_t& operator[](size_t pos);

	uint8_t* begin() { return map; }
	uint8_t* end() { return map + size; }
};
