#pragma once
#include <cstdint>
#include <string>

class md5 {
	uint32_t A = 0x67452301;
	uint32_t B = 0xefcdab89;
	uint32_t C = 0x98badcfe;
	uint32_t D = 0x10325476;
public:
	md5(const std::string& str);
	md5(const char* data, uint64_t length);
	md5(std::istream& stream);

	std::string ToString() const;
private:
	void UpdateHash(char* message);

public:
	bool operator ==(const md5& other) const;
	bool operator !=(const md5& other)const;
};
