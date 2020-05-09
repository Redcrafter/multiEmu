#include "sha1.h"
#include <stdexcept>
#include <cassert>

static uint32_t leftRotate(uint32_t x, uint32_t c) {
	return (x << c) | (x >> (32 - c));
}

static uint32_t reverseBytes(uint32_t val) {
	return
		((val << 24) & 0xFF000000) |
		((val << 8) & 0x00FF0000) |
		((val >> 8) & 0x0000FF00) |
		((val >> 24) & 0x000000FF);
}

sha1::sha1(const std::string& str) : sha1(str.c_str(), str.length()) {}

sha1::sha1(const char* data, uint64_t length) {
	uint8_t message[64];

	for(uint64_t i = 0; i < length; i++) {
		message[i % 64] = data[i];

		if((i + 1) % 64 == 0) {
			UpdateHash((char*)&message);
		}
	}

	int messageIndex = length % 64;

	message[messageIndex] = 0x80; // Append 1 bit, fill with 0
	messageIndex++;

	if(messageIndex > 56) {
		// Not enough space for length
		for(int i = messageIndex; i < 64; ++i) {
			message[i] = 0;
		}
		UpdateHash((char*)&message);
		messageIndex = 0;
	}

	while(messageIndex < 56) {
		message[messageIndex] = 0;
		messageIndex++;
	}

	*reinterpret_cast<uint64_t*>(&message[56]) = ((uint64_t)reverseBytes(length * 8) << 32) | reverseBytes((length * 8) >> 32);
	UpdateHash((char*)&message);
}

static const char hexChars[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

static uint8_t hexToBin(char c) {
	switch(c) {
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			return c - '0';
		case 'A': case 'a': return 10;
		case 'B': case 'b': return 11;
		case 'C': case 'c': return 12;
		case 'D': case 'd': return 13;
		case 'E': case 'e': return 14;
		case 'F': case 'f': return 15;
	}
	throw std::exception("invalid character");
}

sha1 sha1::FromString(const std::string& str) {
	sha1 hash;

	assert(str.size() == 40);

	for(int i = 0; i < 5; i++) {
		int val = 0;
		for(int j = 0; j < 8; j++) {
			val = (val << 4) | hexToBin(str[i * 8 + j]);
		}
		hash.h[i] = val;
	}
	return hash;
}

std::string sha1::ToString() {
	char* str = new char[41];

	uint32_t pos = 0;
	for(int j = 0; j < 5; ++j) {
		uint32_t val = h[j];

		for(int i = 0; i < 4; i++) {
			str[pos + 0] = hexChars[((val >> 24) >> 4) & 0xF];
			str[pos + 1] = hexChars[(val >> 24) & 0xF];

			val <<= 8;
			pos += 2;
		}
	}

	str[40] = '\0';

	return str;
}

void sha1::UpdateHash(char* message) {
	auto M = reinterpret_cast<uint32_t*>(message);

	uint32_t w[80];

	for(int i = 0; i < 16; ++i) {
		w[i] = reverseBytes(M[i]);
	}

	for(int i = 16; i < 80; ++i) {
		w[i] = leftRotate(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
	}

	uint32_t a = h[0];
	uint32_t b = h[1];
	uint32_t c = h[2];
	uint32_t d = h[3];
	uint32_t e = h[4];

	for(int i = 0; i < 80; ++i) {
		uint32_t f, k;
		if(i < 20) {
			f = (b & c) | (~b & d);
			k = 0x5A827999;
		} else if(i < 40) {
			f = b ^ c ^ d;
			k = 0x6ED9EBA1;
		} else if(i < 60) {
			f = (b & c) | (b & d) | (c & d);
			k = 0x8F1BBCDC;
		} else {
			f = b ^ c ^ d;
			k = 0xCA62C1D6;
		}

		uint32_t temp = leftRotate(a, 5) + f + e + k + w[i];
		e = d;
		d = c;
		c = leftRotate(b, 30);
		b = a;
		a = temp;
	}

	h[0] += a;
	h[1] += b;
	h[2] += c;
	h[3] += d;
	h[4] += e;
}

bool operator<(const sha1& left, const sha1& right) {
	if(left.h[0] < right.h[0]) {
		return true;
	}
	if(left.h[0] > right.h[0]) {
		return false;
	}
	
	if(left.h[1] < right.h[1]) {
		return true;
	}
	if(left.h[1] > right.h[1]) {
		return false;
	}
	
	if(left.h[2] < right.h[2]) {
		return true;
	}
	if(left.h[2] > right.h[2]) {
		return false;
	}
	
	if(left.h[3] < right.h[3]) {
		return true;
	}
	if(left.h[3] > right.h[3]) {
		return false;
	}

	if(left.h[4] < right.h[4]) {
		return true;
	}
	if(left.h[4] > right.h[4]) {
		return false;
	}
	return false;
}

bool operator==(const sha1& left, const sha1& right) {
	return left.h[0] == right.h[0] &&
		left.h[1] == right.h[1] &&
		left.h[2] == right.h[2] &&
		left.h[3] == right.h[3] &&
		left.h[4] == right.h[4];
}
