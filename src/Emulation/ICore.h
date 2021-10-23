#pragma once
#include <string>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

#include "../RenderImage.h"
#include "../md5.h"
#include "../saver.h"

struct MemoryDomain {
	int Id;
	std::string Name;
	size_t Size;
};

static void DrawTextureWindow(const RenderImage& texture, float pixelRatio = 1) {
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0);
	ImGui::Begin("Screen", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoTitleBar);
	ImGui::PopStyleVar(2);

	auto windowSize = ImGui::GetWindowSize();

	auto width = texture.GetWidth();
	auto height = texture.GetHeight();

	auto size = ImVec2(width, height) * std::min(windowSize.x / width, windowSize.y / height);

	ImGui::SetCursorPos((windowSize - size) * 0.5);

	texture.BufferImage();
	ImGui::Image(reinterpret_cast<void*>(texture.GetTextureId()), size);

	ImGui::End();
}

class ICore {
  public:
	virtual ~ICore() = default;

	virtual std::string GetName() = 0;
	virtual ImVec2 GetSize() = 0; 
	virtual md5 GetRomHash() = 0;

	virtual std::vector<MemoryDomain> GetMemoryDomains() = 0;
	virtual void WriteMemory(int domain, size_t address, uint8_t val) = 0;
	virtual uint8_t ReadMemory(int domain, size_t address) = 0;

	virtual void Draw() = 0;
	virtual void DrawMenuBar(bool& menuOpen) = 0;

	virtual void SaveState(saver& saver) = 0;
	virtual void LoadState(saver& saver) = 0;

	virtual void LoadRom(const std::string& path) = 0;

	virtual void Reset() = 0;
	virtual void HardReset() = 0;
	virtual void Update() = 0;
};
