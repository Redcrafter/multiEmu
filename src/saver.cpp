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
