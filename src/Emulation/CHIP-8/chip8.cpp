#include "chip8.h"

#include <cstdint>
#include <cstring>
#include <fstream>

#include "../../Input.h"

static Input::Mapper inputMapper ("Chip-8", {
	{"0", 0,  { GLFW_KEY_1, 0 } },
	{"1", 1,  { GLFW_KEY_2, 0 } },
	{"2", 2,  { GLFW_KEY_3, 0 } },
	{"3", 3,  { GLFW_KEY_4, 0 } },
	{"4", 4,  { GLFW_KEY_Q, 0 } },
	{"5", 5,  { GLFW_KEY_W, 0 } },
	{"6", 6,  { GLFW_KEY_E, 0 } },
	{"7", 7,  { GLFW_KEY_R, 0 } },
	{"8", 8,  { GLFW_KEY_A, 0 } },
	{"9", 9,  { GLFW_KEY_S, 0 } },
	{"A", 10, { GLFW_KEY_D, 0 } },
	{"B", 11, { GLFW_KEY_F, 0 } },
	{"C", 12, { GLFW_KEY_Y, 0 } },
	{"D", 13, { GLFW_KEY_X, 0 } },
	{"E", 14, { GLFW_KEY_C, 0 } },
	{"F", 15, { GLFW_KEY_V, 0 } },
});

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
	stream.read((char*)&memory[0x200], len);
}

void Chip8::Reset() {
	PC = 0x200;
	I = 0;
	SP = -1;

	V.fill(0);
	memory.fill(0);
	stack.fill(0);
	gfx.fill(0);

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
					gfx.fill(0);
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
				// VF is written after the result so the flag survives when VX is VF
				case 0x4: {
					auto flag = (vx + vy) >> 8;
					vx += vy;
					V[0xF] = flag;
					break;
				}
				case 0x5: {
					auto flag = (vx >= vy);
					vx -= vy;
					V[0xF] = flag;
					break;
				}
				case 0x6: {
					auto flag = vx & 1;
					vx >>= 1;
					V[0xF] = flag;
					break;
				}
				case 0x7: {
					auto flag = (vy >= vx);
					vx = vy - vx;
					V[0xF] = flag;
					break;
				}
				case 0xE: {
					auto flag = (vx >> 7);
					vx <<= 1;
					V[0xF] = flag;
					break;
				}
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
			// the start position wraps around the screen, the rest of the sprite is clipped at the edges
			for(auto y = 0; y < (opcode & 0xF); y++) {
				auto row = memory[I + y];
				auto py = vy % 32 + y;
				if(py >= 32) {
					break;
				}
				for(auto x = 0; x < 8; x++) {
					auto px = vx % 64 + x;
					if((row & 0x80) != 0 && px < 64) {
						auto pos = px + py * 64;
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
				// only the low nibble names a key; the mapper asserts on ids it does not know
				case 0x9E:
					if(inputMapper.GetKey(vx & 0xF)) {
						PC += 2;
					}
					break;
				case 0xA1:
					if(!inputMapper.GetKey(vx & 0xF)) {
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
					// the original hardware moves on when the key is released, not when it goes down
					bool released = false;
					for(int i = 0; i < 16; i++) {
						if(inputMapper.GetKeyUp(i)) {
							vx = i;
							released = true;
							break;
						}
					}
					if(!released) {
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
