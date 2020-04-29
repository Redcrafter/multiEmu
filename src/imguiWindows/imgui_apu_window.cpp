#include "imgui_apu_window.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "../imgui/imgui.h"
#include "../imgui/imgui_internal.h"

static const char* names[] = {"Pulse1", "Pulse2", "Triangle", "Noise", "DMC"};

ApuWindow::ApuWindow(std::string title) : Title(std::move(title)) {}

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

		const auto DrawList = window->DrawList;
		const auto col = ImGui::GetColorU32(ImVec4(1, 1, 1, 1));

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

			const auto width = inner_bb.GetWidth();
			const auto height = inner_bb.GetHeight();

			const auto pps = inner_bb.GetWidth() / (float)available;
			const auto pph = inner_bb.GetHeight() / 15.0;
			auto lastY = inner_bb.Min.y;

			switch(i) {
				case 0: {
					if(apu->pulse1.enabled && !(apu->pulse1.lengthCounter == 0 || apu->pulse1.timerPeriod < 8 || apu->pulse1.timerPeriod > 0x7FF)) {
						int val = apu->pulse1.dutyCycle;
						double dutyLen = val == 0 ? 0.125 : val * 0.25;

						float x = inner_bb.Min.x;
						float w = width * (apu->pulse1.timerPeriod * 16.0 / available);

						// test
						float off = fmod(width / 2, w);
						x += off;
						
						float h = inner_bb.Min.y;
						if(apu->pulse1.envelopeEnabled) {
							h += (height / 15) * apu->pulse1.constantVolume;
						} else {
							h += (height / 15) * apu->pulse1.envelopeVolume;
						}

						if(x - w * dutyLen <= inner_bb.Min.x) {
							DrawList->PathLineTo(ImVec2(inner_bb.Min.x, h));
						} else {
							DrawList->PathLineTo(inner_bb.Min);
							DrawList->PathLineTo(ImVec2(x - w * dutyLen, inner_bb.Min.y));
							DrawList->PathLineTo(ImVec2(x - w * dutyLen, h));
						}
						DrawList->PathLineTo(ImVec2(x, h));
						DrawList->PathLineTo(ImVec2(x, inner_bb.Min.y));

						while(true) {
							x += w * (1 - dutyLen);
							if(x > inner_bb.Max.x) {
								DrawList->PathLineTo(ImVec2(inner_bb.Max.x, inner_bb.Min.y));
								break;
							}

							DrawList->PathLineTo(ImVec2(x, inner_bb.Min.y));
							DrawList->PathLineTo(ImVec2(x, h));

							x += w * dutyLen;
							if(x > inner_bb.Max.x) {
								DrawList->PathLineTo(ImVec2(inner_bb.Max.x, h));
								break;
							}

							DrawList->PathLineTo(ImVec2(x, h));
							DrawList->PathLineTo(ImVec2(x, inner_bb.Min.y));
						}
					}

					DrawList->PathStroke(col, false, 2);
					break;
				}
				case 1: {
					if(!(!apu->pulse2.enabled || apu->pulse2.lengthCounter == 0 || apu->pulse2.timerPeriod < 8 || apu->pulse2.timerPeriod > 0x7FF)) {
						int val = apu->pulse2.dutyCycle;
						double dutyLen = val == 0 ? 0.125 : val * 0.25;

						float x = inner_bb.Min.x;
						float w = width * (apu->pulse2.timerPeriod * 16.0 / available);
						
						float off = fmod(width / 2, w);
						x += off;

						float h = inner_bb.Min.y;
						if(apu->pulse2.envelopeEnabled) {
							h += (height / 15) * apu->pulse2.constantVolume;
						} else {
							h += (height / 15) * apu->pulse2.envelopeVolume;
						}
						if(x - w * dutyLen <= inner_bb.Min.x) {
							DrawList->PathLineTo(ImVec2(inner_bb.Min.x, h));
						} else {
							DrawList->PathLineTo(inner_bb.Min);
							DrawList->PathLineTo(ImVec2(x - w * dutyLen, inner_bb.Min.y));
							DrawList->PathLineTo(ImVec2(x - w * dutyLen, h));
						}
						DrawList->PathLineTo(ImVec2(x, h));
						DrawList->PathLineTo(ImVec2(x, inner_bb.Min.y));

						while(true) {
							x += w * (1 - dutyLen);
							if(x > inner_bb.Max.x) {
								DrawList->PathLineTo(ImVec2(inner_bb.Max.x, inner_bb.Min.y));
								break;
							}

							DrawList->PathLineTo(ImVec2(x, inner_bb.Min.y));
							DrawList->PathLineTo(ImVec2(x, h));

							x += w * dutyLen;
							if(x > inner_bb.Max.x) {
								DrawList->PathLineTo(ImVec2(inner_bb.Max.x, h));
								break;
							}

							DrawList->PathLineTo(ImVec2(x, h));
							DrawList->PathLineTo(ImVec2(x, inner_bb.Min.y));
						}
					}

					DrawList->PathStroke(col, false, 2);
					break;
				}
				case 2: {
					if(apu->triangle.enabled && apu->triangle.lengthCounter > 0 && apu->triangle.linearCounter > 0 && apu->triangle.timerPeriod >= 2) {
						DrawList->PathLineTo(inner_bb.Min);
						
						float x = inner_bb.Min.x;
						float w = width * (apu->triangle.timerPeriod * 32.0 / available);
						// float off = fmod(width / 2, w);
						// x += off;
						while(true) {
							x += w / 2;
							if(x > inner_bb.Max.x) {
								DrawList->PathLineTo(ImVec2(inner_bb.Max.x, inner_bb.Min.y + height * (1 - (x - inner_bb.Max.x) / (w / 2))));
								break;
							}

							DrawList->PathLineTo(ImVec2(x, inner_bb.Max.y));

							x += w / 2;
							if(x > inner_bb.Max.x) {
								DrawList->PathLineTo(ImVec2(inner_bb.Max.x, inner_bb.Min.y + height * ((x - inner_bb.Max.x) / (w / 2))));
								break;
							}

							DrawList->PathLineTo(ImVec2(x, inner_bb.Min.y));
						}
					}

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
