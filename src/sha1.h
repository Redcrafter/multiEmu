#pragma once
#include <cstdint>
#include <string>

class sha1 {
  private:
	uint32_t h[5] = {
		0x67452301,
		0xEFCDAB89,
		0x98BADCFE,
		0x10325476,
		0xC3D2E1F0
	};

  public:
	sha1() {};
	sha1(const std::string& str);
	sha1(const char* data, uint64_t length);

	friend bool operator<(const sha1& left, const sha1& right);
	friend bool operator==(const sha1& left, const sha1& right);
	friend bool operator!=(const sha1& left, const sha1& right);

	static sha1 FromString(const std::string& str);
	std::string ToString();

  private:
	void UpdateHash(char* message);
};
