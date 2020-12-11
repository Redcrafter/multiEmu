#include "MemoryMapped.h"

#include <cassert>
#include <fstream>

#ifdef _MSC_VER
// Window
#include <windows.h>
#else
// Linux
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
// #include <sys/types.h>
#include <unistd.h>

#include "fs.h"
#endif

MemoryMapped::MemoryMapped(const std::string& filePath, size_t size) {
#ifdef _MSC_VER
    file = CreateFileA(filePath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_ALWAYS, FILE_FLAG_RANDOM_ACCESS, nullptr);
    if (!file) {
        throw std::runtime_error("Error opening the file");
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
#else
    if (!fs::exists(filePath)) {
        // Create file with given size
        std::ofstream ofs(filePath, std::ios::binary | std::ios::out);
        ofs.seekp(size - 1);
        ofs.put(0);
    }

    auto fileSize = fs::file_size(filePath);

    if (fileSize != size) {
        throw std::runtime_error("Invalid mapped file size");
    }

    auto file = open(filePath.c_str(), O_RDWR);
    if (file == -1) {
        throw std::runtime_error("Error opening the file");
    }

    map = (uint8_t*)mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, file, 0);
    close(file);
    if (map == MAP_FAILED) {
        throw std::runtime_error("Error mmapping the file");
    }
#endif
}

MemoryMapped::~MemoryMapped() {
#ifdef _MSC_VER
    UnmapViewOfFile(map);

    if (mappedFile) {
        CloseHandle(mappedFile);
        mappedFile = nullptr;
    }

    CloseHandle(file);
#else
    munmap(map, size);
#endif
    map = nullptr;
}

uint8_t& MemoryMapped::operator[](size_t pos) {
    assert(pos < size);

    return map[pos];
}
