#include "logger.h"
#include <imgui_internal.h>

Logger logger {};

Logger::Logger() {
	AutoScroll = true;
	Clear();
}

void Logger::Clear() {
	Buf.clear();
	LineOffsets.clear();
	LineOffsets.push_back(0);
}

void Logger::LogScreen(const char* fmt, ...) {
	va_list args;

	va_start(args, fmt);
	const auto size = vsnprintf(nullptr, 0, fmt, args) + 1;
	va_end(args);

	const auto buf = new char[size];

	va_start(args, fmt);
	vsnprintf(buf, size, fmt, args);
	va_end(args);

	if(buf[size - 2] == '\n') {
		buf[size - 2] = 0;
	}

	logItems.push_back(ScreenLogItem { buf, std::chrono::steady_clock::now() });

	delete[] buf;
}

void Logger::Log(const char* fmt, ...) {
	int old_size = Buf.size();
	va_list args;

	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);

	va_start(args, fmt);
	Buf.appendfv(fmt, args);
	va_end(args);

	for(int new_size = Buf.size(); old_size < new_size; old_size++)
		if(Buf[old_size] == '\n')
			LineOffsets.push_back(old_size + 1);
}

void Logger::DrawWindow() {
	if(!Show) {
		return;
	}
	ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
	if(!ImGui::Begin("Log", &Show)) {
		ImGui::End();
		return;
	}

	// Options menu
	if(ImGui::BeginPopup("Options")) {
		ImGui::Checkbox("Auto-scroll", &AutoScroll);
		ImGui::EndPopup();
	}

	// Main window
	if(ImGui::Button("Options"))
		ImGui::OpenPopup("Options");
	ImGui::SameLine();
	bool clear = ImGui::Button("Clear");
	// ImGui::SameLine();
	// bool copy = ImGui::Button("Copy");
	ImGui::SameLine();
	Filter.Draw("Filter", -100.0f);

	ImGui::Separator();
	ImGui::BeginChild("scrolling", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

	if(clear)
		Clear();

	// if (copy) ImGui::LogToClipboard();

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	const char* buf = Buf.begin();
	const char* buf_end = Buf.end();
	if(Filter.IsActive()) {
		// In this example we don't use the clipper when Filter is enabled.
		// This is because we don't have random access on the result on our filter.
		// A real application processing logs with ten of thousands of entries may want to store the result of search/filter.
		// especially if the filtering function is not trivial (e.g. reg-exp).
		for(int line_no = 0; line_no < LineOffsets.Size; line_no++) {
			const char* line_start = buf + LineOffsets[line_no];
			const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
			if(Filter.PassFilter(line_start, line_end))
				ImGui::TextUnformatted(line_start, line_end);
		}
	} else {
		// The simplest and easy way to display the entire buffer:
		//   ImGui::TextUnformatted(buf_begin, buf_end);
		// And it'll just work. TextUnformatted() has specialization for large blob of text and will fast-forward to skip non-visible lines.
		// Here we instead demonstrate using the clipper to only process lines that are within the visible area.
		// If you have tens of thousands of items and their processing cost is non-negligible, coarse clipping them on your side is recommended.
		// Using ImGuiListClipper requires A) random access into your data, and B) items all being the  same height,
		// both of which we can handle since we have an array pointing to the beginning of each line of text.
		// When using the filter (in the block of code above) we don't have random access into the data to display anymore, which is why we don't use the clipper.
		// Storing or skimming through the search result would make it possible (and would be recommended if you want to search through tens of thousands of entries)
		ImGuiListClipper clipper;
		clipper.Begin(LineOffsets.Size);
		while(clipper.Step()) {
			for(int line_no = clipper.DisplayStart; line_no < clipper.DisplayEnd; line_no++) {
				const char* line_start = buf + LineOffsets[line_no];
				const char* line_end = (line_no + 1 < LineOffsets.Size) ? (buf + LineOffsets[line_no + 1] - 1) : buf_end;
				ImGui::TextUnformatted(line_start, line_end);
			}
		}
		clipper.End();
	}
	ImGui::PopStyleVar();

	if(AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
		ImGui::SetScrollHereY(1.0f);

	ImGui::EndChild();
	ImGui::End();
}

void Logger::DrawScreen() {
	if(logItems.empty()) {
		return;
	}
	std::string temp;

	auto now = std::chrono::steady_clock::now();
	for(size_t i = 0; i < logItems.size(); ++i) {
		const auto item = logItems[i];

		auto passed = std::chrono::duration_cast<std::chrono::seconds>(now - item.creationTime);
		if(passed.count() >= 10) {
			logItems.erase(logItems.begin() + i);
			i--;
		} else {
			if(i > 0) {
				temp += "\n";
			}
			temp += item.string;
		}
	}

	if(logItems.empty()) {
		return;
	}

	auto padding = ImGui::GetStyle().FramePadding;
	auto textHeight = ImGui::GetTextLineHeight() * logItems.size();

	auto viewport = ImGui::GetMainViewport();
    auto pos = viewport->Pos;
    auto size = viewport->Size;
    auto drawList = ImGui::GetForegroundDrawList(viewport);

	drawList->AddText(ImVec2(pos.x + padding.x, pos.y + size.y - textHeight - padding.y), ImGui::GetColorU32(ImGuiCol_Text), temp.c_str());

	// const float fontSize = ImGui::GetFontSize();
	// auto textSize = ImGui::GetFont()->CalcTextSizeA(fontSize, FLT_MAX, 0, temp.c_str());
	// w->DrawList->AddText(nullptr, fontSize, ImVec2(w->Pos.x + padding.x, w->Pos.y + w->Size.y - textSize.y - padding.y), ImGui::GetColorU32(ImGuiCol_Text), temp.c_str());
}
