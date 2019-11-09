#include "imgui_tas_editor.h"
#include "../imgui/imgui.h"

void TasEditor::DrawWindow() {
	if(!open) {
		return;
	}
	if(ImGui::Begin(Title.c_str(), &open, ImGuiWindowFlags_NoScrollbar)) {
		if(ImGui::BeginMenuBar()) {
			ImGui::EndMenuBar();
		}

		ImGui::End();
	}
}

TasEditor::TasEditor(std::string Title) : Title(std::move(Title)) { }
