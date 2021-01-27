#include "disassembler.h"

#include <map>

#include "imgui.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_internal.h"

#include "logger.h"
#include "nativefiledialog/nfd.h"

namespace Nes {

constexpr int CYCLES_CROSS_PAGE_ADDS_ONE = 1;
constexpr int CYCLES_BRANCH_TAKEN_ADDS_ONE = 2;

const opcode lookup[] = {
	{BRK, IMP, 7, 0}, {ORA, IZX, 6, 0}, {KIL, IMM, 0}, {SLO, IZX, 8, 0}, {NOP, ZP0, 3}, {ORA, ZP0, 3}, {ASL, ZP0, 5}, {SLO, ZP0, 5}, {PHP, IMP, 3}, {ORA, IMM, 2, 0}, {ASL, IMP, 2}, {ANC, IMM, 2, 0}, {NOP, ABS, 4, 0}, {ORA, ABS, 4, 0}, {ASL, ABS, 6, 0}, {SLO, ABS, 6, 0},
	{BPL, REL, 2, 3}, {ORA, IZY, 5, 1}, {KIL, IMM, 0}, {SLO, IZY, 8, 0}, {NOP, ZPX, 4}, {ORA, ZPX, 4}, {ASL, ZPX, 6}, {SLO, ZPX, 6}, {CLC, IMP, 2}, {ORA, ABY, 4, 1}, {NOP, IMP, 2}, {SLO, ABY, 7, 0}, {NOP, ABX, 4, 1}, {ORA, ABX, 4, 1}, {ASL, ABX, 7, 0}, {SLO, ABX, 7, 0},
	{JSR, ABS, 6, 0}, {AND, IZX, 6, 0}, {KIL, IMM, 0}, {RLA, IZX, 8, 0}, {BIT, ZP0, 3}, {AND, ZP0, 3}, {ROL, ZP0, 5}, {RLA, ZP0, 5}, {PLP, IMP, 4}, {AND, IMM, 2, 0}, {ROL, IMP, 2}, {ANC, IMM, 2, 0}, {BIT, ABS, 4, 0}, {AND, ABS, 4, 0}, {ROL, ABS, 6, 0}, {RLA, ABS, 6, 0},
	{BMI, REL, 2, 3}, {AND, IZY, 5, 1}, {KIL, IMM, 0}, {RLA, IZY, 8, 0}, {NOP, ZPX, 4}, {AND, ZPX, 4}, {ROL, ZPX, 6}, {RLA, ZPX, 6}, {SEC, IMP, 2}, {AND, ABY, 4, 1}, {NOP, IMP, 2}, {RLA, ABY, 7, 0}, {NOP, ABX, 4, 1}, {AND, ABX, 4, 1}, {ROL, ABX, 7, 0}, {RLA, ABX, 7, 0},
	{RTI, IMP, 6, 0}, {EOR, IZX, 6, 0}, {KIL, IMM, 0}, {SRE, IZX, 8, 0}, {NOP, ZP0, 3}, {EOR, ZP0, 3}, {LSR, ZP0, 5}, {SRE, ZP0, 5}, {PHA, IMP, 3}, {EOR, IMM, 2, 0}, {LSR, IMP, 2}, {ALR, IMM, 2, 0}, {JMP, ABS, 3, 0}, {EOR, ABS, 4, 0}, {LSR, ABS, 6, 0}, {SRE, ABS, 6, 0},
	{BVC, REL, 2, 3}, {EOR, IZY, 5, 1}, {KIL, IMM, 0}, {SRE, IZY, 8, 0}, {NOP, ZPX, 4}, {EOR, ZPX, 4}, {LSR, ZPX, 6}, {SRE, ZPX, 6}, {CLI, IMP, 2}, {EOR, ABY, 4, 1}, {NOP, IMP, 2}, {SRE, ABY, 7, 0}, {NOP, ABX, 4, 1}, {EOR, ABX, 4, 1}, {LSR, ABX, 7, 0}, {SRE, ABX, 7, 0},
	{RTS, IMP, 6, 0}, {ADC, IZX, 6, 0}, {KIL, IMM, 0}, {RRA, IZX, 8, 0}, {NOP, ZP0, 3}, {ADC, ZP0, 3}, {ROR, ZP0, 5}, {RRA, ZP0, 5}, {PLA, IMP, 4}, {ADC, IMM, 2, 0}, {ROR, IMP, 2}, {ARR, IMM, 2, 0}, {JMP, IND, 5, 0}, {ADC, ABS, 4, 0}, {ROR, ABS, 6, 0}, {RRA, ABS, 6, 0},
	{BVS, REL, 2, 3}, {ADC, IZY, 5, 1}, {KIL, IMM, 0}, {RRA, IZY, 8, 0}, {NOP, ZPX, 4}, {ADC, ZPX, 4}, {ROR, ZPX, 6}, {RRA, ZPX, 6}, {SEI, IMP, 2}, {ADC, ABY, 4, 1}, {NOP, IMP, 2}, {RRA, ABY, 7, 0}, {NOP, ABX, 4, 1}, {ADC, ABX, 4, 1}, {ROR, ABX, 7, 0}, {RRA, ABX, 7, 0},
	{NOP, IMM, 2, 0}, {STA, IZX, 6, 0}, {NOP, IMM, 2}, {SAX, IZX, 6, 0}, {STY, ZP0, 3}, {STA, ZP0, 3}, {STX, ZP0, 3}, {SAX, ZP0, 3}, {DEY, IMP, 2}, {NOP, IMM, 2, 0}, {TXA, IMP, 2}, {XAA, IMM, 2, 0}, {STY, ABS, 4, 0}, {STA, ABS, 4, 0}, {STX, ABS, 4, 0}, {SAX, ABS, 4, 0},
	{BCC, REL, 2, 3}, {STA, IZY, 6, 0}, {KIL, IMM, 0}, {AHX, IZY, 6, 0}, {STY, ZPX, 4}, {STA, ZPX, 4}, {STX, ZPY, 4}, {SAX, ZPY, 4}, {TYA, IMP, 2}, {STA, ABY, 5, 1}, {TXS, IMP, 2}, {TAS, ABY, 5, 0}, {SHY, ABX, 5, 0}, {STA, ABX, 5, 0}, {SHX, ABY, 5, 0}, {AHX, ABY, 5, 0},
	{LDY, IMM, 2, 0}, {LDA, IZX, 6, 0}, {LDX, IMM, 2}, {LAX, IZX, 6, 0}, {LDY, ZP0, 3}, {LDA, ZP0, 3}, {LDX, ZP0, 3}, {LAX, ZP0, 3}, {TAY, IMP, 2}, {LDA, IMM, 2, 0}, {TAX, IMP, 2}, {LAX, IMM, 2, 0}, {LDY, ABS, 4, 0}, {LDA, ABS, 4, 0}, {LDX, ABS, 4, 0}, {LAX, ABS, 4, 0},
	{BCS, REL, 2, 3}, {LDA, IZY, 5, 1}, {KIL, IMM, 0}, {LAX, IZY, 5, 1}, {LDY, ZPX, 4}, {LDA, ZPX, 4}, {LDX, ZPY, 4}, {LAX, ZPY, 4}, {CLV, IMP, 2}, {LDA, ABY, 4, 1}, {TSX, IMP, 2}, {LAS, ABY, 4, 1}, {LDY, ABX, 4, 1}, {LDA, ABX, 4, 1}, {LDX, ABY, 4, 1}, {LAX, ABY, 4, 1},
	{CPY, IMM, 2, 0}, {CMP, IZX, 6, 0}, {NOP, IMM, 2}, {DCP, IZX, 8, 0}, {CPY, ZP0, 3}, {CMP, ZP0, 3}, {DEC, ZP0, 5}, {DCP, ZP0, 5}, {INY, IMP, 2}, {CMP, IMM, 2, 0}, {DEX, IMP, 2}, {AXS, IMM, 2, 0}, {CPY, ABS, 4, 0}, {CMP, ABS, 4, 0}, {DEC, ABS, 6, 0}, {DCP, ABS, 6, 0},
	{BNE, REL, 2, 3}, {CMP, IZY, 5, 1}, {KIL, IMM, 0}, {DCP, IZY, 8, 0}, {NOP, ZPX, 4}, {CMP, ZPX, 4}, {DEC, ZPX, 6}, {DCP, ZPX, 6}, {CLD, IMP, 2}, {CMP, ABY, 4, 1}, {NOP, IMP, 2}, {DCP, ABY, 7, 0}, {NOP, ABX, 4, 1}, {CMP, ABX, 4, 1}, {DEC, ABX, 7, 0}, {DCP, ABX, 7, 0},
	{CPX, IMM, 2, 0}, {SBC, IZX, 6, 0}, {NOP, IMM, 2}, {ISC, IZX, 8, 0}, {CPX, ZP0, 3}, {SBC, ZP0, 3}, {INC, ZP0, 5}, {ISC, ZP0, 5}, {INX, IMP, 2}, {SBC, IMM, 2, 0}, {NOP, IMP, 2}, {SBC, IMM, 2, 0}, {CPX, ABS, 4, 0}, {SBC, ABS, 4, 0}, {INC, ABS, 6, 0}, {ISC, ABS, 6, 0},
	{BEQ, REL, 2, 3}, {SBC, IZY, 5, 1}, {KIL, IMM, 0}, {ISC, IZY, 8, 0}, {NOP, ZPX, 4}, {SBC, ZPX, 4}, {INC, ZPX, 6}, {ISC, ZPX, 6}, {SED, IMP, 2}, {SBC, ABY, 4, 1}, {NOP, IMP, 2}, {ISC, ABY, 7, 0}, {NOP, ABX, 4, 1}, {SBC, ABX, 4, 1}, {INC, ABX, 7, 0}, {ISC, ABX, 7, 0},
};

static bool isIllegal(uint8_t opCode, const opcode& instr) {
	if((opCode & 3) == 3) {
		return true;
	}

	switch(instr.instruction) {
		case SHY:
		case SHX:
			return true;
		case NOP: return opCode != 0xEA;
	}

	return false;
}

void assertBreak(bool expression) {
	if(!expression) {
		// __debugbreak();
	}
}

static void displayInstruction(const Element& el) {
	ImGui::TableSetColumnIndex(0);
	ImGui::Text("0x%04X", el.address);

	if(el.data) {
		ImGui::TableSetColumnIndex(1);
		ImGui::Text("%02X", el.opcode);

		ImGui::TableSetColumnIndex(2);
		ImGui::Text(".byte");
		return;
	}

	auto instr = lookup[el.opcode];
	
	int size = 0;

	ImGui::TableSetColumnIndex(2);

	ImGui::TextColored(ImVec4(0x56 / 255.0, 0x9c / 255.0, 0xd6 / 255.0, 1), "%s ", InstructionNames[instr.instruction]);
	ImGui::SameLine();

	uint16_t operand16 = el.op1 | el.op2 << 8;
	
	switch(instr.addrMode) {
		case IMP:
			size = 1;
			break;
		case IMM:
			size = 2;
			ImGui::Text("#$%02X", el.op1);
			break;
		case ZP0:
			size = 2;
			ImGui::Text("$%02X", el.op1);
			break;
		case ZPX:
			size = 2;
			ImGui::Text("$%02X,X", el.op1);
			break;
		case ZPY:
			size = 2;
			ImGui::Text("$%02X,Y", el.op1);
			break;
		case REL: {
			size = 2;
			// conditional branch
			uint16_t res = el.address + 2;
			if(el.op1 & 0x80) {
				res += (0xFF00 | el.op1);
			} else {
				res += el.op1;
			}
			ImGui::Text("$%04X", res);
			break;
		}
		case ABS:
			size = 3;

			// TODO: make text clickable
			// if(instr.instruction == JSR) { }

			ImGui::Text("$%04X", operand16);
			break;
		case ABX:
			size = 3;
			ImGui::Text("$%04X,X", operand16);
			break;
		case ABY:
			size = 3;
			ImGui::Text("$%04X,Y", operand16);
			break;
		case IND:
			size = 3;
			ImGui::Text("($%04X)", operand16);
			break;
		case IZX:
			size = 2;
			ImGui::Text("($%02X,X)", el.op1);
			break;
		case IZY:
			size = 2;
			ImGui::Text("($%02X),Y", el.op1);
			break;
		default: break;
	}

	ImGui::TableSetColumnIndex(1);
	switch(size) {
		case 1: 
			ImGui::Text("%02X", el.opcode);
			break;
		case 2:
			ImGui::Text("%02X %02X", el.opcode, el.op1);
			break;
		case 3:
			ImGui::Text("%02X %02X %02X", el.opcode, el.op1, el.op2);
			break;
		default: break;
	}

	ImGui::TableSetColumnIndex(3);
	switch(instr.addrMode) {
		case ABS:
		case ABX:
		case ABY:
			switch(operand16) {
				case 0x2000: ImGui::Text("PPU Control"); break;
				case 0x2001: ImGui::Text("PPU Mask"); break;
				case 0x2002: ImGui::Text("PPU status"); break;
				case 0x2003: ImGui::Text("Oam address"); break;
				case 0x2004: ImGui::Text("Oam data"); break;
				case 0x2005: ImGui::Text("PPU scroll"); break;
				case 0x2006: ImGui::Text("VRAM address select"); break;
				case 0x2007: ImGui::Text("VRAM data"); break;

				case 0x4000: ImGui::Text("Audio -> Square 1 Duty/Volume"); break;
				case 0x4001: ImGui::Text("Audio -> Square 1 Sweep"); break;
				case 0x4002: ImGui::Text("Audio -> Square 1 Low"); break;
				case 0x4003: ImGui::Text("Audio -> Square 1 High"); break;

				case 0x4004: ImGui::Text("Audio -> Square 2 Duty/Volume"); break;
				case 0x4005: ImGui::Text("Audio -> Square 2 Sweep"); break;
				case 0x4006: ImGui::Text("Audio -> Square 2 Low"); break;
				case 0x4007: ImGui::Text("Audio -> Square 2 High"); break;

				case 0x4008: ImGui::Text("Audio -> Triangle Linear"); break;
				case 0x4009: ImGui::Text("Audio -> Triangle unused"); break;
				case 0x400A: ImGui::Text("Audio -> Triangle Low"); break;
				case 0x400B: ImGui::Text("Audio -> Triangle High"); break;

				case 0x400C: ImGui::Text("Audio -> Noise Volume"); break;
				case 0x400E: ImGui::Text("Audio -> Noise Period/Shape"); break;
				case 0x400F: ImGui::Text("Audio -> Noise Length"); break;

				case 0x4010: ImGui::Text("Audio -> DMC Mode/Frequency"); break;
				case 0x4011: ImGui::Text("Audio -> DMC Raw"); break;
				case 0x4012: ImGui::Text("Audio -> DMC Start address"); break;
				case 0x4013: ImGui::Text("Audio -> DMC Length"); break;

				case 0x4014: ImGui::Text("Sprite DMA trigger"); break;

				case 0x4015: ImGui::Text("IRQ status / Sound enable"); break;
				case 0x4016: ImGui::Text("Joystick 1"); break;
				case 0x4017: ImGui::Text("Josystick 2/Apu frame counter"); break;
			}
			break;
		default:
			/*if(!el.callers.empty()) {
				ImGui::Text("Called from %i places", el.callers.size());
			}*/
			break;
	}
}

static int InstructionSize(const AddressingModes mode) {
	switch(mode) {
		case IMP: return 1;
		case IMM:
		case ZP0:
		case ZPX:
		case ZPY:
		case IZX:
		case IZY:
		case REL: return 2;
		case ABS:
		case ABX:
		case ABY:
		case IND: return 3;
		default: throw std::logic_error("unreachable");
	}
}

class NesDisassembler {
	std::vector<uint8_t> prg;
	std::map<uint16_t, Element> explored;

	std::vector<uint16_t> locations;

	uint16_t mask = 0;

public:
	NesDisassembler(std::vector<uint8_t> data) : prg(std::move(data)) {
		assertBreak(this->prg.size() == 0x4000 || this->prg.size() == 0.8000);
		mask = prg.size() - 1;

		locations.push_back(read(0xFFFA) | read(0xFFFB) << 8);
		locations.push_back(read(0xFFFC) | read(0xFFFD) << 8);
		locations.push_back(read(0xFFFE) | read(0xFFFF) << 8);
	}

private:
	uint8_t read(uint16_t pos) {
		// assert(pos >= 0x8000);
		return prg[pos & mask];
	}

public:
	void DisasmSingle(uint16_t pc) { // uint32_t prgAddress
		// should be in memory somewhere
		assert(pc >= 0x8000);

		if(explored.count(pc)) {
			// position already visited
			return;
		}

		auto opcode = read(pc);
		const uint8_t op1 = read(pc + 1);
		const uint8_t op2 = read(pc + 2);

		auto instr = lookup[opcode];

		if(isIllegal(opcode, instr)) {
			logger.Log("Decompiler warning: Illegal Instruction '%s' at %04X", InstructionNames[opcode], pc);
			return;
		}

		explored[pc] = { pc, opcode, op1, op2 };

		switch(instr.instruction) {
			case RTI:
			case RTS: // source already explored // can be ignored
				return;
			case JMP: {
				uint16_t addr = op1 | read(pc + 2) << 8;

				if(instr.addrMode == IND) {
					// jump to position form memory
					if(addr < 0x8000) {
						// position somewhere in memory
						return;
					}
					// TODO: should this even be allowed?
					// simulate page error
					addr = read(addr) | read((addr & 0xFF00) | ((addr + 1) & 0xFF)) << 8;
				}

				explored[pc].jumpAddr = addr;
				locations.push_back(addr);
				return;
			}
			case JSR: {
				uint16_t addr = op1 | op2 << 8;
				/*if(explored.count(addr)) {
					explored[addr].callers.push_back(current_addr);
				}*/
				locations.push_back(addr);
				explored[pc].jumpAddr = addr;
				break;
			}
			default:
				break;
		}

		if(instr.addrMode == REL) {
			// conditional branch
			uint16_t res = pc + 2;
			if(op1 & 0x80) {
				res += (0xFF00 | op1);
			} else {
				res += op1;
			}
			// take branch
			locations.push_back(res);
			explored[pc].jumpAddr = res;
		}

		locations.push_back(pc + InstructionSize(instr.addrMode));
	}

	void DisasmNes() {
		while(!locations.empty()) {
			auto addr = locations.back();
			locations.pop_back();
			DisasmSingle(addr);
		}
	}

	auto GenData() {
		uint32_t pos = 0x8000;

		std::vector<Element> out;
		
		out.reserve(explored.size());
		for(auto& value : explored) {
			if(value.first != pos) {
				if(value.first != 0xC000) {
					// printf("Gap from %04X to %04X size: %i bytes \n", pos, value.first, value.first - pos);
					for(int i = pos; i < value.first; ++i) {
						out.push_back({ (uint16_t)i, read(i), 0, 0, 0, true });
					}
				}
				pos = value.first;
			}
			pos += InstructionSize(lookup[value.second.opcode].addrMode);

			out.push_back(value.second);
		}

		return out;
	}
};

void DisassemblerWindow::Open(const std::vector<uint8_t>& data) {
	open = true;

	if(data.size() == 0x4000 || data.size() == 0x8000) {
		NesDisassembler disasm(data);

		disasm.DisasmNes();

		lines = disasm.GenData();
	} else {
		logger.Log("Dissassembly currently only works for games without memory mapping");
	}
}

void DisassemblerWindow::save(const std::string& path) {
	std::ofstream file(path);

	char buffer[128];

	for(auto line : lines) {
		auto name = InstructionNames[line.opcode];
		std::snprintf(buffer, sizeof(buffer), "0x%04X %s", line.address, name);
		file << buffer;
	}
}

void DisassemblerWindow::DrawWindow() {
	if(!open) {
		return;
	}

	if(!ImGui::Begin(title.c_str(), &open, ImGuiWindowFlags_MenuBar)) {
		ImGui::End();
		return;
	}

	if(lines.empty()) {
		ImGui::Text("Can't disassemble big roms because of bank switching");
		ImGui::End();
		return;
	}

	if(ImGui::BeginMenuBar()) {
		if(ImGui::BeginMenu("Options")) {
			// ImGui::Checkbox("Display jumps", &displayJumps);

			if(ImGui::MenuItem("Export")) {
				std::string path;
				NFD::SaveDialog({  }, "./", path, (GLFWwindow*)ImGui::GetMainViewport()->PlatformHandle);
				save(path);
			}
			
			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	if(ImGui::BeginTable("##testTable", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ColumnsWidthFixed)) {
		ImGui::TableSetupColumn("Address");
		ImGui::TableSetupColumn("Bytes");
		ImGui::TableSetupColumn("Disassembly");
		ImGui::TableSetupColumn("Notes");
		ImGui::TableHeadersRow();

		ImGuiListClipper clipper;
		clipper.Begin(lines.size());
		
		while(clipper.Step()) {
			for(int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
				const auto& item = lines[i];
				
				ImGui::TableNextRow();

				displayInstruction(item);
			}
		}

		ImGui::EndTable();
	}

	ImGui::End();
}

}
