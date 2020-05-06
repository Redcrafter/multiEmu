#include "imgui_memory_editor.h"

#ifdef _MSC_VER
#define _PRISizeT   "I"
#define ImSnprintf(buffer, bufferSize, frmt, ...)  _snprintf_s(buffer, bufferSize, bufferSize, frmt, __VA_ARGS__)
#define ImSprintf sprintf_s
#define ImSscanf sscanf_s

#else
#define _PRISizeT   "z"
#define ImSnprintf  snprintf
#define ImSprintf sprintf
#define ImSscanf sscanf

#endif

const char* descs[] = {"Int8", "Uint8", "Int16", "Uint16", "Int32", "Uint32", "Int64", "Uint64", "Float", "Double"};
const size_t sizes[] = {1, 1, 2, 2, 4, 4, 8, 8, 4, 8};
// const char* descs[] = {"Bin", "Dec", "Hex"};

static bool IsBigEndian() {
	uint16_t x = 1;
	char c[2];
	memcpy(c, &x, 2);
	return c[0] != 0;
}

static void* EndianessCopyBigEndian(void* _dst, void* _src, size_t s, int is_little_endian) {
	if(is_little_endian) {
		uint8_t* dst = (uint8_t*)_dst;
		uint8_t* src = (uint8_t*)_src + s - 1;
		for(int i = 0, n = (int)s; i < n; ++i)
			memcpy(dst++, src--, 1);
		return _dst;
	} else {
		return memcpy(_dst, _src, s);
	}
}

static void* EndianessCopyLittleEndian(void* _dst, void* _src, size_t s, int is_little_endian) {
	if(is_little_endian) {
		return memcpy(_dst, _src, s);
	} else {
		uint8_t* dst = (uint8_t*)_dst;
		uint8_t* src = (uint8_t*)_src + s - 1;
		for(int i = 0, n = (int)s; i < n; ++i)
			memcpy(dst++, src--, 1);
		return _dst;
	}
}

static const char* FormatBinary(const uint8_t* buf, int width) {
	IM_ASSERT(width <= 64);
	size_t out_n = 0;
	static char out_buf[64 + 8 + 1];
	int n = width / 8;
	for(int j = n - 1; j >= 0; --j) {
		for(int i = 0; i < 8; ++i)
			out_buf[out_n++] = (buf[j] & (1 << (7 - i))) ? '1' : '0';
		out_buf[out_n++] = ' ';
	}
	IM_ASSERT(out_n < IM_ARRAYSIZE(out_buf));
	out_buf[out_n] = 0;
	return out_buf;
}

MemoryEditor::MemoryEditor(std::string title) : Title(std::move(title)) {
}

void MemoryEditor::GotoAddrAndHighlight(size_t addr_min, size_t addr_max) {
	GotoAddr = addr_min;
	HighlightMin = addr_min;
	HighlightMax = addr_max;
}

void MemoryEditor::DrawWindow() {
	if(!open || !bus) {
		return;
	}

	Sizes s;
	CalcSizes(s, GetDomainSize(), 0);
	ImGui::SetNextWindowSizeConstraints(ImVec2(0.0f, 0.0f), ImVec2(s.WindowWidth, FLT_MAX));

	if(ImGui::Begin(Title.c_str(), &open, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar)) {
		if(ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows) && ImGui::IsMouseClicked(1)) {
			ImGui::OpenPopup("context");
		}

		if(ImGui::BeginMenuBar()) {
			if(ImGui::BeginMenu("Memory Domain")) {
				if(ImGui::MenuItem("RAM")) {
					domain = CpuRam;
				}
				if(ImGui::MenuItem("Cpu Bus")) {
					domain = CpuBus;
				}
				if(ImGui::MenuItem("Ppu Bus")) {
					domain = PpuBus;
				}
				if(ImGui::MenuItem("CIRam (Nametables)")) {
					domain = CIRam;
				}
				if(ImGui::MenuItem("OAM")) {
					domain = Oam;
				}
				if(ImGui::MenuItem("Prg Rom")) {
					domain = PrgRom;
				}
				if(ImGui::MenuItem("Chr Rom")) {
					domain = ChrRom;
				}

				ImGui::EndMenu();
			}

			ImGui::EndMenuBar();
		}

		const auto memorySize = GetDomainSize();
		DrawContents(memorySize);
		if(ContentsWidthChanged) {
			CalcSizes(s, memorySize, 0);
			ImGui::SetWindowSize(ImVec2(s.WindowWidth, ImGui::GetWindowSize().y));
		}
	}
	ImGui::End();
}

bool MemoryEditor::HighlightFn(size_t addr) const {
	return false; // For now
}

uint8_t MemoryEditor::ReadFn(size_t addr) const {
	switch(domain) {
		case CpuRam: return bus->CpuRam[addr];
		case CpuBus: return bus->CpuRead(addr, true);
		case PpuBus: return bus->ppu.ppuRead(addr, true);
		case CIRam: return bus->ppu.vram[addr];
		case Oam: return bus->ppu.pOAM[addr];
		case PrgRom: return bus->cartridge->prg[addr];
		case ChrRom: return bus->cartridge->chr[addr];
		default: ;
	}
	return 0;
}

void MemoryEditor::WriteFn(size_t addr, uint8_t val) {
	switch(domain) {
		case CpuRam:
			bus->CpuRam[addr] = val;
			break;
		case CIRam:
			bus->ppu.vram[addr] = val;
			break;
		case Oam:
			bus->ppu.pOAM[addr] = val;
			break;
	}
}

uint32_t MemoryEditor::GetDomainSize() const {
	switch(domain) {
		case CpuRam: return 0x800;
		case CpuBus: return 0x10000;
		case PpuBus: return 0x8000;
		case CIRam: return 0x800;
		case Oam: return 0x100;
		case PrgRom: return bus->cartridge->prg.size();
		case ChrRom: return bus->cartridge->chr.size();
	}

	return 0;
}

void MemoryEditor::DrawContents(size_t mem_size) {
	const size_t base_display_addr = 0;

	Sizes s;
	CalcSizes(s, mem_size, base_display_addr);
	ImGuiStyle& style = ImGui::GetStyle();

	// We begin into our scrolling region with the 'ImGuiWindowFlags_NoMove' in order to prevent click from moving the window.
	// This is used as a facility since our main click detection code doesn't assign an ActiveId so the click would normally be caught as a window-move.
	const float height_separator = style.ItemSpacing.y;
	float footer_height = 0;
	if(OptShowOptions)
		footer_height += height_separator + ImGui::GetFrameHeightWithSpacing() * 1;
	if(OptShowDataPreview)
		footer_height += height_separator + ImGui::GetFrameHeightWithSpacing() * 1 + ImGui::GetTextLineHeightWithSpacing() * 3;
	ImGui::BeginChild("##scrolling", ImVec2(0, -footer_height), false, ImGuiWindowFlags_NoMove);
	ImDrawList* draw_list = ImGui::GetWindowDrawList();

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

	const int line_total_count = (int)((mem_size + Cols - 1) / Cols);
	ImGuiListClipper clipper(line_total_count, s.LineHeight);
	const size_t visible_start_addr = clipper.DisplayStart * Cols;
	const size_t visible_end_addr = clipper.DisplayEnd * Cols;

	bool data_next = false;

	if(ReadOnly || DataEditingAddr >= mem_size)
		DataEditingAddr = (size_t)-1;
	if(DataPreviewAddr >= mem_size)
		DataPreviewAddr = (size_t)-1;

	size_t preview_data_type_size = OptShowDataPreview ? sizes[PreviewDataType] : 0;

	size_t data_editing_addr_backup = DataEditingAddr;
	size_t data_editing_addr_next = (size_t)-1;
	if(DataEditingAddr != (size_t)-1) {
		// Move cursor but only apply on next frame so scrolling with be synchronized (because currently we can't change the scrolling while the window is being rendered)
		if(ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_UpArrow)) && DataEditingAddr >= (size_t)Cols) {
			data_editing_addr_next = DataEditingAddr - Cols;
			DataEditingTakeFocus = true;
		} else if(ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_DownArrow)) && DataEditingAddr < mem_size - Cols) {
			data_editing_addr_next = DataEditingAddr + Cols;
			DataEditingTakeFocus = true;
		} else if(ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_LeftArrow)) && DataEditingAddr > 0) {
			data_editing_addr_next = DataEditingAddr - 1;
			DataEditingTakeFocus = true;
		} else if(ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_RightArrow)) && DataEditingAddr < mem_size - 1) {
			data_editing_addr_next = DataEditingAddr + 1;
			DataEditingTakeFocus = true;
		}
	}
	if(data_editing_addr_next != (size_t)-1 && (data_editing_addr_next / Cols) != (data_editing_addr_backup / Cols)) {
		// Track cursor movements
		const int scroll_offset = ((int)(data_editing_addr_next / Cols) - (int)(data_editing_addr_backup / Cols));
		const bool scroll_desired = (scroll_offset < 0 && data_editing_addr_next < visible_start_addr + Cols * 2) || (scroll_offset > 0 && data_editing_addr_next > visible_end_addr - Cols * 2);
		if(scroll_desired)
			ImGui::SetScrollY(ImGui::GetScrollY() + scroll_offset * s.LineHeight);
	}

	// Draw vertical separator
	ImVec2 window_pos = ImGui::GetWindowPos();
	if(OptShowAscii)
		draw_list->AddLine(ImVec2(window_pos.x + s.PosAsciiStart - s.GlyphWidth, window_pos.y), ImVec2(window_pos.x + s.PosAsciiStart - s.GlyphWidth, window_pos.y + 9999), ImGui::GetColorU32(ImGuiCol_Border));

	const ImU32 color_text = ImGui::GetColorU32(ImGuiCol_Text);
	const ImU32 color_disabled = OptGreyOutZeroes ? ImGui::GetColorU32(ImGuiCol_TextDisabled) : color_text;

	const char* format_address = OptUpperCaseHex ? "%0*" _PRISizeT "X: " : "%0*" _PRISizeT "x: ";
	const char* format_data = OptUpperCaseHex ? "%0*" _PRISizeT "X" : "%0*" _PRISizeT "x";
	const char* format_range = OptUpperCaseHex ? "Range %0*" _PRISizeT "X..%0*" _PRISizeT "X" : "Range %0*" _PRISizeT "x..%0*" _PRISizeT "x";
	const char* format_byte = OptUpperCaseHex ? "%02X" : "%02x";
	const char* format_byte_space = OptUpperCaseHex ? "%02X " : "%02x ";

	// display only visible lines
	for(int line_i = clipper.DisplayStart; line_i < clipper.DisplayEnd; line_i++) {
		size_t addr = (size_t)(line_i * Cols);
		ImGui::Text(format_address, s.AddrDigitsCount, base_display_addr + addr);

		// Draw Hexadecimal
		for(int n = 0; n < Cols && addr < mem_size; n++, addr++) {
			float byte_pos_x = s.PosHexStart + s.HexCellWidth * n;
			if(OptMidColsCount > 0)
				byte_pos_x += (float)(n / OptMidColsCount) * s.SpacingBetweenMidCols;
			ImGui::SameLine(byte_pos_x);

			// Draw highlight
			bool is_highlight_from_user_range = (addr >= HighlightMin && addr < HighlightMax);
			bool is_highlight_from_user_func = HighlightFn(addr);
			bool is_highlight_from_preview = (addr >= DataPreviewAddr && addr < DataPreviewAddr + preview_data_type_size);
			if(is_highlight_from_user_range || is_highlight_from_user_func || is_highlight_from_preview) {
				ImVec2 pos = ImGui::GetCursorScreenPos();
				float highlight_width = s.GlyphWidth * 2;
				bool is_next_byte_highlighted = (addr + 1 < mem_size) && ((HighlightMax != (size_t)-1 && addr + 1 < HighlightMax) || HighlightFn(addr + 1));
				if(is_next_byte_highlighted || (n + 1 == Cols)) {
					highlight_width = s.HexCellWidth;
					if(OptMidColsCount > 0 && n > 0 && (n + 1) < Cols && ((n + 1) % OptMidColsCount) == 0)
						highlight_width += s.SpacingBetweenMidCols;
				}
				draw_list->AddRectFilled(pos, ImVec2(pos.x + highlight_width, pos.y + s.LineHeight), HighlightColor);
			}

			if(DataEditingAddr == addr) {
				// Display text input on current byte
				bool data_write = false;
				ImGui::PushID((void*)addr);
				if(DataEditingTakeFocus) {
					ImGui::SetKeyboardFocusHere();
					ImGui::CaptureKeyboardFromApp(true);
					ImSprintf(AddrInputBuf, format_data, s.AddrDigitsCount, base_display_addr + addr);
					ImSprintf(DataInputBuf, format_byte, ReadFn(addr));
				}
				ImGui::PushItemWidth(s.GlyphWidth * 2);
				struct UserData {
					// FIXME: We should have a way to retrieve the text edit cursor position more easily in the API, this is rather tedious. This is such a ugly mess we may be better off not using InputText() at all here.
					static int Callback(ImGuiInputTextCallbackData* data) {
						UserData* user_data = (UserData*)data->UserData;
						if(!data->HasSelection())
							user_data->CursorPos = data->CursorPos;
						if(data->SelectionStart == 0 && data->SelectionEnd == data->BufTextLen) {
							// When not editing a byte, always rewrite its content (this is a bit tricky, since InputText technically "owns" the master copy of the buffer we edit it in there)
							data->DeleteChars(0, data->BufTextLen);
							data->InsertChars(0, user_data->CurrentBufOverwrite);
							data->SelectionStart = 0;
							data->SelectionEnd = data->CursorPos = 2;
						}
						return 0;
					}

					char CurrentBufOverwrite[3]; // Input
					int CursorPos;               // Output
				};
				UserData user_data;
				user_data.CursorPos = -1;
				ImSprintf(user_data.CurrentBufOverwrite, format_byte, ReadFn(addr));
				ImGuiInputTextFlags flags = ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_NoHorizontalScroll | ImGuiInputTextFlags_AlwaysInsertMode | ImGuiInputTextFlags_CallbackAlways;
				if(ImGui::InputText("##data", DataInputBuf, 32, flags, UserData::Callback, &user_data))
					data_write = data_next = true;
				else if(!DataEditingTakeFocus && !ImGui::IsItemActive())
					DataEditingAddr = data_editing_addr_next = (size_t)-1;
				DataEditingTakeFocus = false;
				ImGui::PopItemWidth();
				if(user_data.CursorPos >= 2)
					data_write = data_next = true;
				if(data_editing_addr_next != (size_t)-1)
					data_write = data_next = false;
				unsigned int data_input_value = 0;
				if(data_write && ImSscanf(DataInputBuf, "%X", &data_input_value) == 1) {
					WriteFn(addr, (uint8_t)data_input_value);
				}
				ImGui::PopID();
			} else {
				// NB: The trailing space is not visible but ensure there's no gap that the mouse cannot click on.
				uint8_t b = ReadFn(addr);

				if(OptShowHexII) {
					if((b >= 32 && b < 128))
						ImGui::Text(".%c ", b);
					else if(b == 0xFF && OptGreyOutZeroes)
						ImGui::TextDisabled("## ");
					else if(b == 0x00)
						ImGui::Text("   ");
					else
						ImGui::Text(format_byte_space, b);
				} else {
					if(b == 0 && OptGreyOutZeroes)
						ImGui::TextDisabled("00 ");
					else
						ImGui::Text(format_byte_space, b);
				}
				if(!ReadOnly && ImGui::IsItemHovered() && ImGui::IsMouseClicked(0)) {
					DataEditingTakeFocus = true;
					data_editing_addr_next = addr;
				}
			}
		}

		if(OptShowAscii) {
			// Draw ASCII values
			ImGui::SameLine(s.PosAsciiStart);
			ImVec2 pos = ImGui::GetCursorScreenPos();
			addr = line_i * Cols;
			ImGui::PushID(line_i);
			if(ImGui::InvisibleButton("ascii", ImVec2(s.PosAsciiEnd - s.PosAsciiStart, s.LineHeight))) {
				DataEditingAddr = DataPreviewAddr = addr + (size_t)((ImGui::GetIO().MousePos.x - pos.x) / s.GlyphWidth);
				DataEditingTakeFocus = true;
			}
			ImGui::PopID();
			for(int n = 0; n < Cols && addr < mem_size; n++, addr++) {
				if(addr == DataEditingAddr) {
					draw_list->AddRectFilled(pos, ImVec2(pos.x + s.GlyphWidth, pos.y + s.LineHeight), ImGui::GetColorU32(ImGuiCol_FrameBg));
					draw_list->AddRectFilled(pos, ImVec2(pos.x + s.GlyphWidth, pos.y + s.LineHeight), ImGui::GetColorU32(ImGuiCol_TextSelectedBg));
				}
				unsigned char c = ReadFn(addr);
				char display_c = (c < 32 || c >= 128) ? '.' : c;
				draw_list->AddText(pos, (display_c == '.') ? color_disabled : color_text, &display_c, &display_c + 1);
				pos.x += s.GlyphWidth;
			}
		}
	}
	clipper.End();
	ImGui::PopStyleVar(2);
	ImGui::EndChild();

	if(data_next && DataEditingAddr < mem_size) {
		DataEditingAddr = DataPreviewAddr = DataEditingAddr + 1;
		DataEditingTakeFocus = true;
	} else if(data_editing_addr_next != (size_t)-1) {
		DataEditingAddr = DataPreviewAddr = data_editing_addr_next;
	}

	bool next_show_data_preview = OptShowDataPreview;
	if(OptShowOptions) {
		ImGui::Separator();

		// Options menu

		if(ImGui::Button("Options"))
			ImGui::OpenPopup("context");
		if(ImGui::BeginPopup("context")) {
			ImGui::PushItemWidth(56);
			if(ImGui::DragInt("##cols", &Cols, 0.2f, 4, 32, "%d cols")) { ContentsWidthChanged = true; }
			ImGui::PopItemWidth();
			ImGui::Checkbox("Show Data Preview", &next_show_data_preview);
			ImGui::Checkbox("Show HexII", &OptShowHexII);
			if(ImGui::Checkbox("Show Ascii", &OptShowAscii)) { ContentsWidthChanged = true; }
			ImGui::Checkbox("Grey out zeroes", &OptGreyOutZeroes);
			ImGui::Checkbox("Uppercase Hex", &OptUpperCaseHex);

			ImGui::EndPopup();
		}

		ImGui::SameLine();
		ImGui::Text(format_range, s.AddrDigitsCount, base_display_addr, s.AddrDigitsCount, base_display_addr + mem_size - 1);
		ImGui::SameLine();
		ImGui::PushItemWidth((s.AddrDigitsCount + 1) * s.GlyphWidth + style.FramePadding.x * 2.0f);
		if(ImGui::InputText("##addr", AddrInputBuf, 32, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
			size_t goto_addr;
			if(ImSscanf(AddrInputBuf, "%" _PRISizeT "X", &goto_addr) == 1) {
				GotoAddr = goto_addr - base_display_addr;
				HighlightMin = HighlightMax = (size_t)-1;
			}
		}
		ImGui::PopItemWidth();

		if(GotoAddr != (size_t)-1) {
			if(GotoAddr < mem_size) {
				ImGui::BeginChild("##scrolling");
				ImGui::SetScrollFromPosY(ImGui::GetCursorStartPos().y + (GotoAddr / Cols) * ImGui::GetTextLineHeight());
				ImGui::EndChild();
				DataEditingAddr = DataPreviewAddr = GotoAddr;
				DataEditingTakeFocus = true;
			}
			GotoAddr = (size_t)-1;
		}
	}

	if(OptShowDataPreview) {
		ImGui::Separator();
		ImGui::AlignTextToFramePadding();
		ImGui::Text("Preview as:");
		ImGui::SameLine();
		ImGui::PushItemWidth((s.GlyphWidth * 10.0f) + style.FramePadding.x * 2.0f + style.ItemInnerSpacing.x);
		if(ImGui::BeginCombo("##combo_type", descs[PreviewDataType], ImGuiComboFlags_HeightLargest)) {
			for(int n = 0; n < DataType_COUNT; n++) {
				if(ImGui::Selectable(descs[n], PreviewDataType == n)) {
					PreviewDataType = (DataType)n;
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PopItemWidth();
		ImGui::SameLine();
		ImGui::PushItemWidth((s.GlyphWidth * 6.0f) + style.FramePadding.x * 2.0f + style.ItemInnerSpacing.x);
		ImGui::Combo("##combo_endianess", &PreviewEndianess, "LE\0BE\0\0");
		ImGui::PopItemWidth();

		char buf[128];
		float x = s.GlyphWidth * 6.0f;
		bool has_value = DataPreviewAddr != (size_t)-1;
		if(has_value)
			DisplayPreviewData(DataPreviewAddr, mem_size, PreviewDataType, DataFormat_Dec, buf, (size_t)IM_ARRAYSIZE(buf));
		ImGui::Text("Dec");
		ImGui::SameLine(x);
		ImGui::TextUnformatted(has_value ? buf : "N/A");
		if(has_value)
			DisplayPreviewData(DataPreviewAddr, mem_size, PreviewDataType, DataFormat_Hex, buf, (size_t)IM_ARRAYSIZE(buf));
		ImGui::Text("Hex");
		ImGui::SameLine(x);
		ImGui::TextUnformatted(has_value ? buf : "N/A");
		if(has_value)
			DisplayPreviewData(DataPreviewAddr, mem_size, PreviewDataType, DataFormat_Bin, buf, (size_t)IM_ARRAYSIZE(buf));
		ImGui::Text("Bin");
		ImGui::SameLine(x);
		ImGui::TextUnformatted(has_value ? buf : "N/A");
	}

	OptShowDataPreview = next_show_data_preview;

	// Notify the main window of our ideal child content size (FIXME: we are missing an API to get the contents size from the child)
	ImGui::SetCursorPosX(s.WindowWidth);
}

void MemoryEditor::CalcSizes(Sizes& s, size_t mem_size, size_t base_display_addr) {
	ImGuiStyle& style = ImGui::GetStyle();
	s.AddrDigitsCount = OptAddrDigitsCount;
	if(s.AddrDigitsCount == 0) {
		for(size_t n = base_display_addr + mem_size - 1; n > 0; n >>= 4) {
			s.AddrDigitsCount++;
		}
	}
	s.LineHeight = ImGui::GetTextLineHeight();
	s.GlyphWidth = ImGui::CalcTextSize("F").x + 1;                  // We assume the font is mono-space
	s.HexCellWidth = (float)(int)(s.GlyphWidth * 2.5f);             // "FF " we include trailing space in the width to easily catch clicks everywhere
	s.SpacingBetweenMidCols = (float)(int)(s.HexCellWidth * 0.25f); // Every OptMidColsCount columns we add a bit of extra spacing
	s.PosHexStart = (s.AddrDigitsCount + 2) * s.GlyphWidth;
	s.PosHexEnd = s.PosHexStart + (s.HexCellWidth * Cols);
	s.PosAsciiStart = s.PosAsciiEnd = s.PosHexEnd;
	if(OptShowAscii) {
		s.PosAsciiStart = s.PosHexEnd + s.GlyphWidth * 1;
		if(OptMidColsCount > 0) {
			s.PosAsciiStart += (float)((Cols + OptMidColsCount - 1) / OptMidColsCount) * s.SpacingBetweenMidCols;
		}
		s.PosAsciiEnd = s.PosAsciiStart + Cols * s.GlyphWidth;
	}
	s.WindowWidth = s.PosAsciiEnd + style.ScrollbarSize + style.WindowPadding.x * 2 + s.GlyphWidth;
}

void* MemoryEditor::EndianessCopy(void* dst, void* src, size_t size) const {
	static void* (*fp)(void*, void*, size_t, int) = NULL;
	if(fp == NULL)
		fp = IsBigEndian() ? EndianessCopyBigEndian : EndianessCopyLittleEndian;
	return fp(dst, src, size, PreviewEndianess);
}

void MemoryEditor::DisplayPreviewData(size_t addr, size_t mem_size, DataType data_type, DataFormat data_format, char* out_buf, size_t out_buf_size) const {
	uint8_t buf[8];

	assert(data_type >= 0 && data_type < 10);
	size_t elem_size = sizes[data_type];
	size_t size = addr + elem_size > mem_size ? mem_size - addr : elem_size;

	for(int i = 0, n = (int)size; i < n; ++i) {
		buf[i] = ReadFn(addr + i);
	}

	if(data_format == DataFormat_Bin) {
		uint8_t binbuf[8];
		EndianessCopy(binbuf, buf, size);
		ImSnprintf(out_buf, out_buf_size, "%s", FormatBinary(binbuf, (int)size * 8));
		return;
	}

	out_buf[0] = 0;
	switch(data_type) {
		case DataType_S8: {
			int8_t int8 = 0;
			EndianessCopy(&int8, buf, size);
			if(data_format == DataFormat_Dec) {
				ImSnprintf(out_buf, out_buf_size, "%hhd", int8);
				return;
			}
			if(data_format == DataFormat_Hex) {
				ImSnprintf(out_buf, out_buf_size, "0x%02x", int8 & 0xFF);
				return;
			}
			break;
		}
		case DataType_U8: {
			uint8_t uint8 = 0;
			EndianessCopy(&uint8, buf, size);
			if(data_format == DataFormat_Dec) {
				ImSnprintf(out_buf, out_buf_size, "%hhu", uint8);
				return;
			}
			if(data_format == DataFormat_Hex) {
				ImSnprintf(out_buf, out_buf_size, "0x%02x", uint8 & 0XFF);
				return;
			}
			break;
		}
		case DataType_S16: {
			int16_t int16 = 0;
			EndianessCopy(&int16, buf, size);
			if(data_format == DataFormat_Dec) {
				ImSnprintf(out_buf, out_buf_size, "%hd", int16);
				return;
			}
			if(data_format == DataFormat_Hex) {
				ImSnprintf(out_buf, out_buf_size, "0x%04x", int16 & 0xFFFF);
				return;
			}
			break;
		}
		case DataType_U16: {
			uint16_t uint16 = 0;
			EndianessCopy(&uint16, buf, size);
			if(data_format == DataFormat_Dec) {
				ImSnprintf(out_buf, out_buf_size, "%hu", uint16);
				return;
			}
			if(data_format == DataFormat_Hex) {
				ImSnprintf(out_buf, out_buf_size, "0x%04x", uint16 & 0xFFFF);
				return;
			}
			break;
		}
		case DataType_S32: {
			int32_t int32 = 0;
			EndianessCopy(&int32, buf, size);
			if(data_format == DataFormat_Dec) {
				ImSnprintf(out_buf, out_buf_size, "%d", int32);
				return;
			}
			if(data_format == DataFormat_Hex) {
				ImSnprintf(out_buf, out_buf_size, "0x%08x", int32);
				return;
			}
			break;
		}
		case DataType_U32: {
			uint32_t uint32 = 0;
			EndianessCopy(&uint32, buf, size);
			if(data_format == DataFormat_Dec) {
				ImSnprintf(out_buf, out_buf_size, "%u", uint32);
				return;
			}
			if(data_format == DataFormat_Hex) {
				ImSnprintf(out_buf, out_buf_size, "0x%08x", uint32);
				return;
			}
			break;
		}
		case DataType_S64: {
			int64_t int64 = 0;
			EndianessCopy(&int64, buf, size);
			if(data_format == DataFormat_Dec) {
				ImSnprintf(out_buf, out_buf_size, "%lld", (long long)int64);
				return;
			}
			if(data_format == DataFormat_Hex) {
				ImSnprintf(out_buf, out_buf_size, "0x%016llx", (long long)int64);
				return;
			}
			break;
		}
		case DataType_U64: {
			uint64_t uint64 = 0;
			EndianessCopy(&uint64, buf, size);
			if(data_format == DataFormat_Dec) {
				ImSnprintf(out_buf, out_buf_size, "%llu", (long long)uint64);
				return;
			}
			if(data_format == DataFormat_Hex) {
				ImSnprintf(out_buf, out_buf_size, "0x%016llx", (long long)uint64);
				return;
			}
			break;
		}
		case DataType_Float: {
			float float32 = 0.0f;
			EndianessCopy(&float32, buf, size);
			if(data_format == DataFormat_Dec) {
				ImSnprintf(out_buf, out_buf_size, "%f", float32);
				return;
			}
			if(data_format == DataFormat_Hex) {
				ImSnprintf(out_buf, out_buf_size, "%a", float32);
				return;
			}
			break;
		}
		case DataType_Double: {
			double float64 = 0.0;
			EndianessCopy(&float64, buf, size);
			if(data_format == DataFormat_Dec) {
				ImSnprintf(out_buf, out_buf_size, "%f", float64);
				return;
			}
			if(data_format == DataFormat_Hex) {
				ImSnprintf(out_buf, out_buf_size, "%a", float64);
				return;
			}
			break;
		}
		case DataType_COUNT:
			break;
	}             // Switch
	IM_ASSERT(0); // Shouldn't reach
}
