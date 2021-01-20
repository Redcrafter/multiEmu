#include "cpu_state.h"
#include <imgui.h>

CpuStateWindow::CpuStateWindow(std::string title) : Title(std::move(title)) { }

void CpuStateWindow::Open() {
	open = true;
}

void CpuStateWindow::DrawWindow() {
	if(!open || !cpu) {
		return;
	}

	if(ImGui::Begin(Title.c_str(), &open)) {
		ImGui::Text("PC: $%04X\nSP: $%04X\nA: $%02X\nX: $%02X\nY: $%02X", cpu->PC, cpu->SP, cpu->A, cpu->X, cpu->Y);
		// TODO: print trace
		

	}

	ImGui::End();
}
