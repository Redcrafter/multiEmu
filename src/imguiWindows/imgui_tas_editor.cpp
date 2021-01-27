#include "imgui_tas_editor.h"
#include <imgui.h>

void TasEditor::DrawWindow(int currentFrame) {
	if(!open) {
		return;
	}
	if(ImGui::Begin(Title.c_str(), &open, ImGuiWindowFlags_NoScrollbar)) {
		if(ImGui::BeginMenuBar()) {
			ImGui::EndMenuBar();
		}

		const int count = 2000;
		static bool test[count][8];

		// TODO: scroll to currentFrame
		// TODO: column header
		// TODO: Right side stuff

		ImGui::BeginChild("Inputs"); // maybe group

		ImGui::Columns(9); // Somehow block moving borders
		ImGui::SetColumnWidth(0, 50);
		for(int i = 1; i < 9; ++i) {
			ImGui::SetColumnWidth(i, 20);
		}
		ImGuiListClipper clipper;
		clipper.Begin(count);

		while(clipper.Step()) {
			for(int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i) {
				ImGui::Text("%i", i);
				ImGui::NextColumn();

				for(int j = 0; j < 8; ++j) {
					if(test[i][j]) {
						ImGui::Text("%i", j);
					}
					ImGui::NextColumn();
				}
			}
		}

		ImGui::Columns(1);
		ImGui::EndChild();
	}

	ImGui::End();
}

TasEditor::TasEditor(std::string Title) : Title(std::move(Title)) {
}
