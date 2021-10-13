#pragma once
#include <cstdint>
#include <string>

class md5 {
  private:
	uint32_t h[4] = {
		0x67452301,
		0xefcdab89,
		0x98badcfe,
		0x10325476
	};

  public:
	md5() = default;
	md5(const std::string& str);
	md5(const char* data, uint64_t length);
	md5(std::istream& stream);

	friend bool operator<(const md5& left, const md5& right);
	friend bool operator==(const md5& left, const md5& right);
	friend bool operator!=(const md5& left, const md5& right);

	static md5 FromString(const std::string& str);
	std::string ToString() const;

  private:
	void UpdateHash(char* message);
};
