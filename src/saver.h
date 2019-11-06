#pragma once
#include <cstdint>
#include <vector>
#include <string>

class saver {
private:
	std::vector<uint8_t> data;
public:
	uint32_t readPos = 0;

	saver(std::string path);
	saver();
	~saver();

	void Save(std::string path);
	
	void Write(const char* data, int size);
	void Read(char* data, int size);

	char* ReadString();

	void operator <<(bool val);
	void operator <<(uint8_t val);
	void operator <<(uint16_t val);
	void operator <<(int32_t val);
	void operator <<(uint32_t val);
	void operator <<(uint64_t val);

	void operator <<(std::string str);

	void operator >>(bool& val);
	void operator >>(uint8_t& val);
	void operator >>(uint16_t& val);
	void operator >>(int32_t& val);
	void operator >>(uint32_t& val);
	void operator >>(uint64_t& val);
};
