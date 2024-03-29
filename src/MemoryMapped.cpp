#include "MemoryMapped.h"

#include <cassert>

#include "fs.h"

#ifdef _MSC_VER
#include <windows.h>

static std::string getWinErrorString() {
    WCHAR buffer[1024] = L"";
    char message[1024] = "";

	FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK,
				   nullptr, GetLastError(),
				   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				   buffer, sizeof(buffer) / sizeof(WCHAR), nullptr);
	WideCharToMultiByte(CP_UTF8, 0, buffer, -1, message, sizeof(message), nullptr, nullptr);

	return message;
}

MemoryMapped::MemoryMapped(const std::string& filePath, size_t size) {
	fs::create_directories(fs::path(filePath).parent_path());

	file = CreateFileA(filePath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_FLAG_RANDOM_ACCESS, nullptr);
	if(file == INVALID_HANDLE_VALUE) {
		throw std::runtime_error("Error opening the file: " + getWinErrorString());
	}

	LARGE_INTEGER large;
	large.QuadPart = size;

	SetFilePointerEx(file, large, nullptr, FILE_BEGIN);
	// Sets file size to file pointer
	SetEndOfFile(file);

	mappedFile = CreateFileMapping(file, nullptr, PAGE_READWRITE, 0, 0, nullptr);

	map = (uint8_t*)MapViewOfFile(mappedFile, FILE_MAP_ALL_ACCESS, 0, 0, size);
	if(map == nullptr) {
		throw std::runtime_error("Error mmapping the file");
	}
}

MemoryMapped::~MemoryMapped() {
	UnmapViewOfFile(map);

	if(mappedFile) {
		CloseHandle(mappedFile);
		mappedFile = nullptr;
	}

	CloseHandle(file);
	map = nullptr;
}

#else // Linux
#include <fstream>

#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

MemoryMapped::MemoryMapped(const std::string& filePath, size_t size) {
	fs::create_directories(fs::path(filePath).parent_path());

	if(!fs::exists(filePath)) {
		// Create file with given size
		std::ofstream ofs(filePath, std::ios::binary | std::ios::out);
		ofs.seekp(size - 1);
		ofs.put(0);
	}

	auto fileSize = fs::file_size(filePath);

	if(fileSize != size) {
		throw std::runtime_error("Invalid mapped file size");
	}

	auto file = open(filePath.c_str(), O_RDWR);
	if(file == -1) {
		throw std::runtime_error("Error opening the file");
	}

	map = (uint8_t*)mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0);
	close(file);
	if(map == MAP_FAILED) {
		throw std::runtime_error("Error memory mapping the file");
	}
}

MemoryMapped::~MemoryMapped() {
	munmap(map, size);
	map = nullptr;
}

#endif

uint8_t& MemoryMapped::operator[](size_t pos) {
	assert(pos < size);
    return map[pos];
}

const uint8_t& MemoryMapped::operator[](size_t pos) const {
	assert(pos < size);
	return map[pos];
}
