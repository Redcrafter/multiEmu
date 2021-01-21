#pragma once
#include <cstdint>
#include <stdexcept>

static const char hexChars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

static void printHex(char* buffer, uint8_t val) {
	buffer[0] = hexChars[val >> 4];
	buffer[1] = hexChars[val & 0xF];
}

static void printHex(char* buffer, uint16_t val) {
	buffer[0] = hexChars[val >> 12];
	buffer[1] = hexChars[val >> 8 & 0xF];
	buffer[2] = hexChars[val >> 4 & 0xF];
	buffer[3] = hexChars[val >> 0xF];
}

// outpus 8 hex characters
static void printHex(char* buffer, uint32_t val) {
	for(int i = 0; i < 8; i++) {
		buffer[i] = hexChars[val >> 28];
		val <<= 4;
	}
}

static uint8_t hexToDec(char c) {
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
	throw std::runtime_error("invalid character");
}
