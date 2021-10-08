#include "chip8.h"

#include <cstdint>
#include <cstring>
#include <fstream>

#include "../../Input.h"

static const uint8_t chip8_fontset[] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

namespace Chip8 {

Chip8::Chip8() {
	Reset();
}

void Chip8::LoadRom(const std::string& path) {
	std::ifstream stream(path, std::ios::binary | std::ios::ate);
	auto len = stream.tellg();
	if(len > 0x1000 - 0x200) {
		throw std::runtime_error("File too big");
	}

	stream.seekg(0, std::ios::beg);
	stream.read((char*)(&memory[0x200]), len);
}

void Chip8::Reset() {
	PC = 0x200;
	I = 0;
	SP = -1;

	std::memset(gfx, 0, sizeof(gfx));
	std::memset(V, 0, sizeof(V));
	std::memset(stack, 0, sizeof(stack));
	std::memset(memory.data(), 0, sizeof(memory));

	std::memcpy(memory.data(), chip8_fontset, 80);

	delay_timer = 0;
	sound_timer = 0;
}

void Chip8::Clock() {
	auto opcode = (memory[PC] << 8) | (memory[PC + 1]);
	PC += 2;

	auto& vx = V[(opcode >> 8) & 0xF];
	auto& vy = V[(opcode >> 4) & 0xF];

	switch(opcode & 0xF000) {
		case 0x0000:
			switch(opcode) {
				case 0x00E0: // 00E0: Clears the screen
					std::memset(gfx, 0, sizeof(gfx));
					break;
				case 0x00EE: // 00EE: Returns from a subroutine
					PC = stack[SP];
					SP--;
					break;
				default: // unknown opcode
					break;
			}
			break;
		case 0x1000: // 1NNN: Jumps to address NNN
			PC = opcode & 0xFFF;
			break;
		case 0x2000: // 2NNN: Calls subroutine at NNN
			SP++;
			stack[SP] = PC;
			PC = opcode & 0xFFF;
			break;
		case 0x3000: // 3XNN: Skips the next instruction if VX equals NN
			if(vx == (opcode & 0xFF)) {
				PC += 2;
			}
			break;
		case 0x4000: // 4XNN: Skips the next instruction if VX doesn't equal NN
			if(vx != (opcode & 0xFF)) {
				PC += 2;
			}
			break;
		case 0x5000: // 5XY0: Skips the next instruction if VX equals VY
			if(vx == vy) {
				PC += 2;
			}
			break;
		case 0x6000: // 6XNN: Sets VX to NN
			vx = opcode & 0xFF;
			break;
		case 0x7000: // 7XNN: Adds NN to VX
			vx += opcode & 0xFF;
			break;
		case 0x8000:
			switch(opcode & 0xF) {
				case 0x0: vx = vy; break;
				case 0x1: vx |= vy; break;
				case 0x2: vx &= vy; break;
				case 0x3: vx ^= vy; break;
				case 0x4:
					V[0xF] = (vx + vy) >> 8;
					vx += vy;
					break;
				case 0x5:
					V[0xF] = (vx >= vy);
					vx -= vy;
					break;
				case 0x6:
					V[0xF] = vx & 1;
					vx >>= 1;
					break;
				case 0x7:
					V[0xF] = (vy >= vx);
					vx = vy - vx;
					break;
				case 0xE:
					V[0xF] = (vx >> 7);
					vx <<= 1;
					break;
				default: // unknown opcode
					break;
			}
			break;
		case 0x9000: // 9XY0: Skips the next instruction if VX doesn't equal VY
			if(vx != vy) {
				PC += 2;
			}
			break;
		case 0xA000: // ANNN: Sets I to the address NNN
			I = opcode & 0xFFF;
			break;
		case 0xB000: // BNNN: Jumps to the address NNN plus V0
			PC = V[0] + (opcode & 0xFFF);
			break;
		case 0xC000: // CXNN Sets VX to the result of a bitwise and operation on a random number and NN
			vx = rand() & opcode & 0xFF;
			break;
		case 0xD000: // DXYN: Draws a sprite at coordinate (VX, VY) that has a width of 8 pixels and a height of N pixels
			V[0xF] = 0;
			for(auto y = 0; y < (opcode & 0xF); y++) {
				auto row = memory[I + y];
				for(auto x = 0; x < 8; x++) {
					if((row & 0x80) != 0) {
						auto pos = (vx + x) + (vy + y) * 64;
						if(gfx[pos] == 1) {
							V[0xF] = 1;
						}

						gfx[pos] ^= 1;
					}
					row <<= 1;
				}
			}
			break;
		case 0xE000:
			switch(opcode & 0xFF) {
				case 0x9E:
					if(Input::GetKey(vx)) {
						PC += 2;
					}
					break;
				case 0xA1:
					if(!Input::GetKey(vx)) {
						PC += 2;
					}
					break;
				default:
					break;
			}
			break;
		case 0xF000:
			switch(opcode & 0xFF) {
				case 0x07:
					vx = delay_timer;
					break;
				case 0x0A: {
					bool hasInput = false;
					for(int i = 0; i < 16; i++) {
						if(Input::GetKey(i)) {
							hasInput = true;
							vx = i;
							break;
						}
					}
					if(!hasInput) {
						PC -= 2;
					}
					break;
				}
				case 0x15:
					delay_timer = vx;
					break;
				case 0x18:
					sound_timer = vx;
					break;
				case 0x1E:
					V[0xF] = (I + vx) > 0xFFF;
					I = (I + vx) & 0xFFF;
					break;
				case 0x29:
					I = vx * 5;
					break;
				case 0x33:
					memory[I + 0] = vx / 100;
					memory[I + 1] = (vx / 10) % 10;
					memory[I + 2] = vx % 10;
					break;
				case 0x55:
					for(int i = (opcode >> 8) & 0xF; i >= 0; i--) {
						memory[I + i] = V[i];
					}
					break;
				case 0x65:
					for(int i = (opcode >> 8) & 0xF; i >= 0; i--) {
						V[i] = memory[I + i];
					}
					break;
				default:
					break;
			}
			break;
		default: // unknown opcode
			break;
	}
}

}
