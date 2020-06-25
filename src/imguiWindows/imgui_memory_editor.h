// Mini memory editor for Dear ImGui (to embed in your game/tools)
// Animated GIF: https://twitter.com/ocornut/status/894242704317530112
// Get latest version at http://www.github.com/ocornut/imgui_club
//
// Right-click anywhere to access the Options menu!
// You can adjust the keyboard repeat delay/rate in ImGuiIO.
// The code assume a mono-space font for simplicity! If you don't use the default font, use ImGui::PushFont()/PopFont() to switch to a mono-space font before caling this.
//
// Usage:
//   static MemoryEditor mem_edit_1;                                            // store your state somewhere
//   mem_edit_1.DrawWindow("Memory Editor", mem_block, mem_block_size, 0x0000); // create a window and draw memory editor (if you already have a window, use DrawContents())
//
// Usage:
//   static MemoryEditor mem_edit_2;
//   ImGui::Begin("MyWindow")
//   mem_edit_2.DrawContents(this, sizeof(*this), (size_t)this);
//   ImGui::End();
//
// Changelog:
// - v0.10: initial version
// - v0.11: always refresh active text input with the latest byte from source memory if it's not being edited.
// - v0.12: added OptMidRowsCount to allow extra spacing every XX rows.
// - v0.13: added optional ReadFn/WriteFn handlers to access memory via a function. various warning fixes for 64-bits.
// - v0.14: added GotoAddr member, added GotoAddrAndHighlight() and highlighting. fixed minor scrollbar glitch when resizing.
// - v0.15: added maximum window width. minor optimization.
// - v0.16: added OptGreyOutZeroes option. various sizing fixes when resizing using the "Rows" drag.
// - v0.17: added HighlightFn handler for optional non-contiguous highlighting.
// - v0.18: fixes for displaying 64-bits addresses, fixed mouse click gaps introduced in recent changes, cursor tracking scrolling fixes.
// - v0.19: fixed auto-focus of next byte leaving WantCaptureKeyboard=false for one frame. we now capture the keyboard during that transition.
// - v0.20: added options menu. added OptShowAscii checkbox. added optional HexII display. split Draw() in DrawWindow()/DrawContents(). fixing glyph width. refactoring/cleaning code.
// - v0.21: fixes for using DrawContents() in our own window. fixed HexII to actually be useful and not on the wrong side.
// - v0.22: clicking Ascii view select the byte in the Hex view. Ascii view highlight selection.
// - v0.23: fixed right-arrow triggering a byte write.
// - v0.24: changed DragInt("Rows" to use a %d data format (which is desirable since imgui 1.61).
// - v0.25: fixed wording: all occurrences of "Rows" renamed to "Columns".
// - v0.26: fixed clicking on hex region
// - v0.30: added data preview for common data types
// - v0.31: added OptUpperCaseHex option to select lower/upper casing display [@samhocevar]
// - v0.32: changed signatures to use void* instead of unsigned char*
// - v0.33: added OptShowOptions option to hide all the interactive option setting.
// - v0.34: binary preview now applies endianess setting [@nicolasnoble]
//
// Todo/Bugs:
// - Arrows are being sent to the InputText() about to disappear which for LeftArrow makes the text cursor appear at position 1 for one frame.
// - Using InputText() is awkward and maybe overkill here, consider implementing something custom.

#pragma once
#include <cstdint>
#include <string>
#include <memory>
#include <imgui.h>

#include "Emulation/ICore.h"

class MemoryEditor {
protected:
	enum DataType {
		DataType_S8,
		DataType_U8,
		DataType_S16,
		DataType_U16,
		DataType_S32,
		DataType_U32,
		DataType_S64,
		DataType_U64,
		DataType_Float,
		DataType_Double,
		DataType_COUNT
	};

	enum DataFormat {
		DataFormat_Bin = 0,
		DataFormat_Dec = 1,
		DataFormat_Hex = 2,
		DataFormat_COUNT
	};

	struct Sizes {
		int AddrDigitsCount;
		float LineHeight;
		float GlyphWidth;
		float HexCellWidth;
		float SpacingBetweenMidCols;
		float PosHexStart;
		float PosHexEnd;
		float PosAsciiStart;
		float PosAsciiEnd;
		float WindowWidth;
	};
private:
	std::string Title;
	
	// Settings
	bool ReadOnly = false;           // disable any editing.
	int Cols = 16;                   // number of columns to display.
	bool OptShowOptions = true;      // display options button/context menu. when disabled, options will be locked unless you provide your own UI for them.
	bool OptShowDataPreview = false; // display a footer previewing the decimal/binary/hex/float representation of the currently selected bytes.
	bool OptShowHexII = false;       // display values in HexII representation instead of regular hexadecimal: hide null/zero bytes, ascii values as ".X".
	bool OptShowAscii = true;        // display ASCII representation on the right side.
	bool OptGreyOutZeroes = true;    // display null/zero bytes using the TextDisabled color.
	bool OptUpperCaseHex = true;     // display hexadecimal values as "FF" instead of "ff".
	int OptMidColsCount = 8;         // set to 0 to disable extra spacing between every mid-cols.
	int OptAddrDigitsCount = 0;      // number of addr digits to display (default calculated based on maximum displayed addr).

	ImU32 HighlightColor = IM_COL32(255, 255, 255, 50);             // background color of highlighted bytes.

	ICore* currentCore = nullptr;
	std::vector<MemoryDomain> domains;
	int selectedDomain = 0;
protected:
	bool open = false;

	bool ContentsWidthChanged = false;
	size_t DataPreviewAddr = -1;
	size_t DataEditingAddr = -1;
	bool DataEditingTakeFocus = false;
	char DataInputBuf[32]{};
	char AddrInputBuf[32]{};
	size_t GotoAddr = -1;
	size_t HighlightMin = -1;
	size_t HighlightMax = -1;
	int PreviewEndianess = 0;
	DataType PreviewDataType = DataType_S32;
public:
	MemoryEditor(std::string title);

	void SetCore(ICore* core) {
		currentCore = core;
		domains = core->GetMemoryDomains();
	}
	
	void Open() { open = true; }
	void Close() { open = false; }

	void GotoAddrAndHighlight(size_t addr_min, size_t addr_max);

	// Standalone Memory Editor window
	void DrawWindow();

private:
	bool HighlightFn(size_t addr) const;
	uint8_t ReadFn(size_t addr) const;
	void WriteFn(size_t addr, uint8_t val);

	uint32_t GetDomainSize() const;
protected:
	// Memory Editor contents only
	void DrawContents(size_t mem_size);
	
	void CalcSizes(Sizes& s, size_t mem_size, size_t base_display_addr);

	void* EndianessCopy(void* dst, void* src, size_t size) const;

	void DisplayPreviewData(size_t addr, size_t mem_size, DataType data_type, DataFormat data_format, char* out_buf, size_t out_buf_size) const;
};
