#include "imgui_apu_window.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"

static const char* names[] = {"Pulse1", "Pulse2", "Triangle", "Noise", "DMC"};

ApuWindow::ApuWindow(std::string title) : Title(std::move(title)) {
}

void ApuWindow::DrawWindow(int available) {
	if(!open || !apu) {
		return;
	}

	if(ImGui::Begin(Title.c_str(), &open)) {
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		/*if(window->SkipItems)
			return;*/
		ImGuiContext& g = *GImGui;
		const ImGuiStyle& style = g.Style;

		int startP1 = -1, startP2 = -1, startTriangle = -1;
		for(int i = 0; i < available; ++i) {
			auto val = apu->waveBuffer[i];

			bool done = true;
			if(startP1 == -1) {
				done = false;
				if(val.pulse1 == 0)
					startP1 = i;
			}
			if(startP2 == -1) {
				done = false;
				if(val.pulse2 == 0)
					startP2 = i;
			}
			if(startTriangle == -1) {
				done = false;
				if(val.triangle == 0)
					startTriangle = i;
			}

			if(done) {
				break;
			}
		}

		for(int i = 0; i < 5; ++i) {
			const char* label = names[i];

			const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
			const auto frame_size = ImVec2(ImGui::CalcItemWidth(), 80);

			const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + frame_size);
			const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
			const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0));
			ImGui::ItemSize(total_bb, style.FramePadding.y);
			if(!ImGui::ItemAdd(total_bb, 0, &frame_bb)) {
				break;
			}

			ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

			auto DrawList = window->DrawList;
			auto pps = inner_bb.GetWidth() / (float)available;
			auto pph = inner_bb.GetHeight() / 15;
			auto col = ImGui::GetColorU32(ImVec4(1, 1, 1, 1));

			int lastY = inner_bb.Min.y;
			int last = 0;

			DrawList->PathLineTo(inner_bb.Min);

			switch(i) {
				case 0: {
					if(startP1 != -1) {
						for(int i = startP1; i < available; i += 8) {
							auto val = apu->waveBuffer[i].pulse1;
							if(last != val) {
								auto x = inner_bb.Min.x + pps * (i - startP1);
								auto y = inner_bb.Min.y + pph * val;

								DrawList->PathLineTo(ImVec2(x, lastY));
								DrawList->PathLineTo(ImVec2(x, y));

								lastY = y;
								last = val;
							}
						}
					}

					DrawList->PathLineTo(ImVec2(inner_bb.Max.x, lastY));
					DrawList->PathStroke(col, false, 2);
					break;
				}
				case 1: {
					if(startP2 != -1) {
						for(int i = startP2; i < available; i += 8) {
							auto val = apu->waveBuffer[i].pulse2;
							if(last != val) {
								auto x = inner_bb.Min.x + pps * (i - startP2);
								auto y = inner_bb.Min.y + pph * val;

								DrawList->PathLineTo(ImVec2(x, lastY));
								DrawList->PathLineTo(ImVec2(x, y));

								lastY = y;
								last = val;
							}
						}
					}

					DrawList->PathLineTo(ImVec2(inner_bb.Max.x, lastY));
					DrawList->PathStroke(col, false, 2);
					break;
				}
				case 2: {
					if(startTriangle != -1) {
						for(int i = startTriangle; i < available; i += 8) {
							auto val = apu->waveBuffer[i].triangle;
							if(last != val) {
								auto x = inner_bb.Min.x + pps * (i - startTriangle);
								auto y = inner_bb.Min.y + pph * val;

								DrawList->PathLineTo(ImVec2(x, lastY));
								DrawList->PathLineTo(ImVec2(x, y));

								lastY = y;
								last = val;
							}
						}
					}

					DrawList->PathLineTo(ImVec2(inner_bb.Max.x, lastY));
					DrawList->PathStroke(col, false, 2);
					break;
				}
				case 3: {
					int last = 0;
					DrawList->PathLineTo(inner_bb.Min);

					for(int i = 0; i < available; i++) {
						auto val = apu->waveBuffer[i].noise;
						if(last != val) {
							auto x = inner_bb.Min.x + pps * i;
							auto y = inner_bb.Min.y + pph * val;

							DrawList->PathLineTo(ImVec2(x, lastY));
							DrawList->PathLineTo(ImVec2(x, y));

							lastY = y;
							last = val;
						}
					}

					DrawList->PathLineTo(ImVec2(inner_bb.Max.x, lastY));
					DrawList->PathStroke(col, false, 1);
					break;
				}
				case 4: {
					int last = 0;
					DrawList->PathLineTo(inner_bb.Min);

					for(int i = 0; i < available; i += 2) {
						auto val = apu->waveBuffer[i].dmc;
						if(last != val) {
							auto x = inner_bb.Min.x + pps * i;
							auto y = inner_bb.Min.y + pph * (val / 8);

							DrawList->PathLineTo(ImVec2(x, lastY));
							DrawList->PathLineTo(ImVec2(x, y));

							lastY = y;
							last = val;
						}
					}

					DrawList->PathLineTo(ImVec2(inner_bb.Max.x, lastY));
					DrawList->PathStroke(col, false, 1);
					break;
				}
			}

			if(label_size.x > 0.0f) {
				ImGui::RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, inner_bb.Min.y), label);
			}
		}
	}

	ImGui::End();
}
