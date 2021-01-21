#include "apu_window.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

namespace Nes {

constexpr float cyclesPerFrame = 29780.5;

static ImRect MakeBox(const char* label) {
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	const ImGuiStyle& style = GImGui->Style;


	const ImVec2 label_size = ImGui::CalcTextSize(label, nullptr, true);
	const auto frame_size = ImVec2(ImGui::CalcItemWidth(), 80);

	const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + frame_size);
	const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);
	const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0));
	ImGui::ItemSize(total_bb, style.FramePadding.y);
	if(!ImGui::ItemAdd(total_bb, 0, &frame_bb)) {
		// break;
	}

	ImGui::RenderFrame(frame_bb.Min, frame_bb.Max, ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

	if(label_size.x > 0.0f) {
		ImGui::RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, inner_bb.Min.y), label);
	}

	return inner_bb;
}

void ApuWindow::DrawPulse(const Pulse& pulse, const char* label) const {
	const ImRect inner_bb = MakeBox(label);
	
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	const auto DrawList = window->DrawList;
	const auto width = inner_bb.GetWidth();
	const auto height = inner_bb.GetHeight();
	auto lineColor = ImGui::GetColorU32(ImVec4(1, 1, 1, 1));
	
	if(pulse.enabled && !(pulse.lengthCounter == 0 || pulse.timerPeriod < 8 || pulse.timerPeriod > 0x7FF)) {
		int val = pulse.dutyCycle;
		double dutyLen = val == 0 ? 0.125 : val * 0.25;

		float x = inner_bb.Min.x;
		float w = width * (pulse.timerPeriod * 16.0 / cyclesPerFrame);
		float h = inner_bb.Min.y;
		
		// center duty
		x += fmod(width / 2, w);

		if(pulse.envelopeEnabled) {
			h += (height / 15) * pulse.constantVolume;
		} else {
			h += (height / 15) * pulse.envelopeVolume;
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
	} else {
		DrawList->PathLineTo(inner_bb.Min);
		DrawList->PathLineTo(ImVec2(inner_bb.Max.x, inner_bb.Min.y));
	}

	DrawList->PathStroke(lineColor, false, 2);
}
void ApuWindow::DrawTriangle() const {
	const auto &triangle = apu->triangle;
	const ImRect inner_bb = MakeBox("Triangle");

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	const auto DrawList = window->DrawList;
	const auto width = inner_bb.GetWidth();
	const auto height = inner_bb.GetHeight();
	auto lineColor = ImGui::GetColorU32(ImVec4(1, 1, 1, 1));
	
	if(triangle.enabled && triangle.lengthCounter > 0 && triangle.linearCounter > 0 && triangle.timerPeriod >= 2) {
		float x = inner_bb.Min.x;
		float w = width * (triangle.timerPeriod * 32.0 / cyclesPerFrame);
		
		/*x += fmod(width / 2, w);

		if(x - w / 2 <= inner_bb.Min.x) {
			DrawList->PathLineTo(ImVec2(inner_bb.Min.x, inner_bb.Min.y + height * (1 - (inner_bb.Min.x - (x - w / 2)) / (w / 2))));
		} else {

			DrawList->PathLineTo(ImVec2(x - w / 2, inner_bb.Max.y));
		}
		DrawList->PathLineTo(ImVec2(x, inner_bb.Min.y));
		*/
		DrawList->PathLineTo(inner_bb.Min);
		
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
	} else {
		DrawList->PathLineTo(inner_bb.Min);
		DrawList->PathLineTo(ImVec2(inner_bb.Max.x, inner_bb.Min.y));
	}

	DrawList->PathStroke(lineColor, false, 2);
}
void ApuWindow::DrawNoise() const {
	const int available = apu->lastBufferPos;

	const ImRect inner_bb = MakeBox("Noise");
	
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	const auto DrawList = window->DrawList;
	const auto pps = inner_bb.GetWidth() / (double)available;
	const auto pph = inner_bb.GetHeight() / 15.0;

	auto lastY = inner_bb.Min.y;
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
	DrawList->PathStroke(ImGui::GetColorU32(ImVec4(1, 1, 1, 1)), false, 1);
}
void ApuWindow::DrawDMC() const {
	const int available = apu->lastBufferPos;

	const ImRect inner_bb = MakeBox("DMC");
	
	ImGuiWindow* window = ImGui::GetCurrentWindow();
	const auto DrawList = window->DrawList;
	const auto pps = inner_bb.GetWidth() / (double)available;
	const auto pph = inner_bb.GetHeight() / 15.0;

	auto lastY = inner_bb.Min.y;
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
	DrawList->PathStroke(ImGui::GetColorU32(ImVec4(1, 1, 1, 1)), false, 1);
}

void ApuWindow::DrawVrc6Pulse(const vrc6Pulse& pulse, const char* label) const {
	const ImRect inner_bb = MakeBox(label);

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	const auto DrawList = window->DrawList;
	const auto width = inner_bb.GetWidth();
	const auto height = inner_bb.GetHeight();
	auto lineColor = ImGui::GetColorU32(ImVec4(1, 1, 1, 1));

	if(pulse.enabled) {
		// percentage of up
		double dutyLen = pulse.mode ? 1 : (pulse.dutyCycle + 1) / 16.0;

		// Width of one duty cycle
		float w = width * (pulse.timerPeriod * 16.0 / cyclesPerFrame);
		// Height based on volume
		float h = inner_bb.Min.y + (height / 15.0) * pulse.Volume;

		// current x
		float x = inner_bb.Min.x;
		// center duty
		x += fmod(width / 2, w);

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
	} else {
		DrawList->PathLineTo(inner_bb.Min);
		DrawList->PathLineTo(ImVec2(inner_bb.Max.x, inner_bb.Min.y));
	}

	DrawList->PathStroke(lineColor, false, 2);
}
void ApuWindow::DrawVrc6Saw() const {
	const auto& saw = apu->vrc6Saw;
	const ImRect inner_bb = MakeBox("vrc6 Sawtooth");

	ImGuiWindow* window = ImGui::GetCurrentWindow();
	const auto DrawList = window->DrawList;
	const auto width = inner_bb.GetWidth();
	auto lineColor = ImGui::GetColorU32(ImVec4(1, 1, 1, 1));

	if(saw.enabled) {
		// Width of one duty cycle
		float w = width * (saw.timerPeriod * 16.0 / cyclesPerFrame);
		float height = inner_bb.GetHeight() * (6 * saw.accumRate >> 3 & 0x1F) / 31.0;

		// Height based on volume
		float h = inner_bb.Min.y + height;

		// current x
		float x = inner_bb.Min.x;
		// center duty
		x += fmod(width / 2, w);

		
		DrawList->PathLineTo(ImVec2(inner_bb.Min.x, inner_bb.Min.y + height * (inner_bb.Min.x - (x - w)) / w));
		DrawList->PathLineTo(ImVec2(x, h));
		DrawList->PathLineTo(ImVec2(x, inner_bb.Min.y));

		while(true) {
			x += w;
			if(x > inner_bb.Max.x) {
				DrawList->PathLineTo(ImVec2(inner_bb.Max.x, inner_bb.Min.y + height * (inner_bb.Max.x - (x - w)) / w));
				break;
			}
			
			DrawList->PathLineTo(ImVec2(x, h));
			DrawList->PathLineTo(ImVec2(x, inner_bb.Min.y));
		}
	} else {
		DrawList->PathLineTo(inner_bb.Min);
		DrawList->PathLineTo(ImVec2(inner_bb.Max.x, inner_bb.Min.y));
	}

	DrawList->PathStroke(lineColor, false, 2);
}

ApuWindow::ApuWindow(std::string title) : Title(std::move(title)) {}

void ApuWindow::DrawWindow() {
	if(!open || !apu) {
		return;
	}
	
	ImGui::SetNextWindowSize(ImVec2(500, 500), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSizeConstraints(ImVec2(200, 0), ImVec2(INFINITY, INFINITY));
	if(ImGui::Begin(Title.c_str(), &open)) {
		ImGui::PushItemWidth(ImGui::GetFontSize() * -8);

		DrawPulse(apu->pulse1, "Pulse1");
		DrawPulse(apu->pulse2, "Pulse2");
		DrawTriangle();
		DrawNoise();
		DrawDMC();
		
		if(apu->vrc6) {
			DrawVrc6Pulse(apu->vrc6Pulse1, "vrc6 Pulse1");
			DrawVrc6Pulse(apu->vrc6Pulse2, "vrc6 Pulse2");
			DrawVrc6Saw();
		}
	}

	ImGui::End();
}

}
