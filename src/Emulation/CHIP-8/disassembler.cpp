#include "disassembler.h"

#include <algorithm>
#include <set>

#include <imgui.h>

namespace Chip8 {

static void displayInstruction(const Element& el) {
	ImGui::TableSetColumnIndex(0);
	ImGui::Text("0x%04X", el.address);

	const auto opcode = el.opcode;

	if(el.data) {
		ImGui::TableSetColumnIndex(1);
		ImGui::Text("%02X", el.data);

		ImGui::TableSetColumnIndex(2);
		ImGui::Text(".byte");
	} else {
		ImGui::TableSetColumnIndex(1);
		ImGui::Text("%02X %02X", opcode >> 8, opcode & 0xFF);

		ImGui::TableSetColumnIndex(2);

		auto imm12 = opcode & 0xFFF;
		auto imm8 = opcode & 0xFF;
		auto vx = (opcode >> 8) & 0xF;
		auto vy = (opcode >> 4) & 0xF;

		switch(opcode & 0xF000) {
			case 0x0000:
				switch(opcode) {
					case 0x00E0: ImGui::Text("disp_clear()"); break;
					case 0x00EE: ImGui::Text("return"); break;
					default: ImGui::Text("N/A"); break;
				}
				break;
			case 0x1000: ImGui::Text("goto 0x%03X", imm12); break;
			case 0x2000: ImGui::Text("call 0x%03X", imm12); break;
			case 0x3000: ImGui::Text("if(V%01X == %i)", vx, imm8); break;
			case 0x4000: ImGui::Text("if(V%01X != %i)", vx, imm8); break;
			case 0x9000: ImGui::Text("if(V%01X != V%01X)", vx, vy); break;
			case 0x5000: ImGui::Text("if(V%01X == V%01X)", vx, vy); break;
			case 0x6000: ImGui::Text("V%01X = %i", vx, imm8); break;
			case 0x7000: ImGui::Text("V%01X += %i", vx, imm8); break;
			case 0xA000: ImGui::Text("I  = 0x%03X", imm12); break;
			case 0xB000:
				ImGui::Text("goto V0 + 0x%03X", imm12);
				ImGui::TableSetColumnIndex(3);
				ImGui::Text("Indirekt jump. Incomplete decompilation");
				break;
			case 0xC000: ImGui::Text("V%01X = rand() & %i", vx, imm8); break;
			case 0xD000: ImGui::Text("draw(V%01X, V%01X, %01X)", vx, vy, opcode & 0xF); break;
			case 0x8000:
				switch(opcode & 0xF) {
					case 0: ImGui::Text("V%01X = V%01X", vx, vy); break;
					case 1: ImGui::Text("V%01X |= V%01X", vx, vy); break;
					case 2: ImGui::Text("V%01X &= V%01X", vx, vy); break;
					case 3: ImGui::Text("V%01X ^= V%01X", vx, vy); break;
					case 4: ImGui::Text("V%01X += V%01X", vx, vy); break;
					case 5: ImGui::Text("V%01X -= V%01X", vx, vy); break;
					case 6: ImGui::Text("V%01X >>= 1", vx); break; // Stores the least significant bit of VX in VF and then shifts VX to the right by 1.
					case 7: ImGui::Text("V%01X = V%01X - V%01X", vx, vy, vx); break;
					case 0xE: ImGui::Text("V%01X <<= 1", vx); break; // Stores the most significant bit of VX in VF and then shifts VX to the left by 1.
				}
				break;
			case 0xE000:
				switch(opcode & 0xFF) {
					case 0x9E: ImGui::Text("if(key() == V%01X)", vx); break;
					case 0xA1: ImGui::Text("if(key() != V%01X)", vx); break;
					default: ImGui::Text("N/A");
				}
				break;
			case 0xF000:
				switch(opcode & 0xFF) {
					case 0x07: ImGui::Text("V%01X = get_delay()", vx); break;
					case 0x0A: ImGui::Text("V%01X = get_key()", vx); break;
					case 0x15: ImGui::Text("delay_timer(V%01X)", vx); break;
					case 0x18: ImGui::Text("sound_timer(V%01X)", vx); break;
					case 0x1E: ImGui::Text("I += V%01X", vx); break;
					case 0x29: ImGui::Text("I = V%01X * 5", vx); break;
					case 0x33: ImGui::Text("set_BCD(V%01X, &I)", vx); break;
					case 0x55: ImGui::Text("reg_dump(V%01X, &I)", vx); break;
					case 0x65: ImGui::Text("reg_dump(V%01X, &I)", vx); break;
					default: ImGui::Text("N/A");
				}
				break;
		}
	}
}

void DisassemblerWindow::Open() {
	open = true;
	Update();
}

void DisassemblerWindow::Update() {
	if(!open) return;

	std::set<uint16_t> explored;
	std::vector<uint16_t> locations;
	locations.push_back(0x200);

	while(!locations.empty()) {
		auto addr = locations.back() & 0xFFF;
		locations.pop_back();

		if(explored.count(addr)) continue;
		explored.emplace(addr);

		auto opcode = (chip8.memory[addr] << 8) | (chip8.memory[addr + 1]);

		switch(opcode & 0xF000) {
			case 0x0000:
				if(opcode != 0x00EE) locations.push_back(addr + 2);
				break;
			case 0x1000:
				locations.push_back(opcode & 0xFFF);
				break;
			case 0x2000:
				locations.push_back(addr + 2);
				locations.push_back(opcode & 0xFFF);
				break;
			case 0x3000:
			case 0x4000:
			case 0x5000:
			case 0x9000:
			case 0xE000:
				locations.push_back(addr + 2);
				locations.push_back(addr + 4);
				break;
			case 0xB000: // __debugbreak();
				break;
			default: locations.push_back(addr + 2);
		}

		elements.push_back({ (uint16_t)addr, (uint16_t)opcode, false });
	}

	std::sort(elements.begin(), elements.end(), [](Element& a, Element& b) { return a.address < b.address; });

	auto elPos = 0;
	for(size_t i = 0; i < 0x1000; i++) {
		auto& el = elements[elPos];

		if(i == el.address) {
			i++;
			elPos++;
		} else {
			elements.push_back({ (uint16_t)i, chip8.memory[i], true });
		}
	}

	std::sort(elements.begin(), elements.end(), [](Element& a, Element& b) { return a.address < b.address; });
}

void DisassemblerWindow::DrawWindow() {
	if(!open) {
		return;
	}

	if(ImGui::Begin("Disassembler", &open, ImGuiWindowFlags_MenuBar)) {
		if(ImGui::BeginMenuBar()) {
			if(ImGui::MenuItem("Step")) {
				chip8.Clock();
			}
			/*if(ImGui::BeginMenu("Options")) {
				// ImGui::Checkbox("Display jumps", &displayJumps);

				if(ImGui::MenuItem("Export")) {
					std::string path;
					NFD::SaveDialog({}, "./", path, (GLFWwindow*)ImGui::GetMainViewport()->PlatformHandle);
					save(path);
				}

				ImGui::EndMenu();
			}*/

			ImGui::EndMenuBar();
		}

		ImGui::Text("PC = %04X  I: %04X", chip8.PC, chip8.I);
		for (int i = 0; i < 16; i++) {
			if(i > 0 && i != 8) ImGui::SameLine();
			ImGui::Text("V%X: %02X", i, chip8.V[i]);
		}

		ImGui::BeginChild("##tableScroll");

		if(ImGui::BeginTable("##chip8table", 4, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingFixedFit)) {
			ImGui::TableSetupColumn("Address");
			ImGui::TableSetupColumn("Bytes");
			ImGui::TableSetupColumn("Disassembly");
			ImGui::TableSetupColumn("Notes");
			ImGui::TableHeadersRow();

			ImGuiListClipper clipper;
			clipper.Begin(elements.size());

			while(clipper.Step()) {
				for(int i = clipper.DisplayStart; i < clipper.DisplayEnd; i++) {
					ImGui::TableNextRow();

					if(chip8.PC == elements[i].address) {
						ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImColor(255, 0, 0));
					}
					displayInstruction(elements[i]);
				}
			}

			ImGui::EndTable();
		}

        ImGui::EndChild();
	}

	ImGui::End();
}

}
