#pragma once
#include <string>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>

#include <nlohmann/json.hpp>

#include "../Texture.h"
#include "../md5.h"
#include "../settings.h"

struct MemoryDomain {
	int Id;
	std::string Name;
	size_t Size;
};

static void DrawTextureWindow(const Texture& texture, float pixelAspectRatio) {
    if(Settings::GameInWindow) {
	    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	    ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0);
	    ImGui::Begin("Screen", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoTitleBar);
	    ImGui::PopStyleVar(2);
	} else {
	    ImGuiWindowFlags window_flags =
		    ImGuiWindowFlags_NoTitleBar |
		    ImGuiWindowFlags_NoResize |
		    ImGuiWindowFlags_NoMove |
		    ImGuiWindowFlags_NoScrollbar |
		    ImGuiWindowFlags_NoScrollWithMouse |
		    ImGuiWindowFlags_NoCollapse |
		    ImGuiWindowFlags_NoBackground |
		    ImGuiWindowFlags_NoSavedSettings |
		    ImGuiWindowFlags_NoBringToFrontOnFocus |
		    ImGuiWindowFlags_NoNavFocus |
		    ImGuiWindowFlags_NoDocking;

	    ImGuiViewport* viewport = ImGui::GetMainViewport();
	    ImGui::SetNextWindowPos(viewport->WorkPos);
	    ImGui::SetNextWindowSize(viewport->WorkSize);
	    ImGui::SetNextWindowViewport(viewport->ID);
	    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	    ImGui::Begin("NES", nullptr, window_flags);
	    ImGui::PopStyleVar(3);
    }

	auto windowSize = ImGui::GetWindowSize();

    auto width = texture.GetWidth() * pixelAspectRatio;
	auto height = texture.GetHeight();
	auto size = ImVec2(width, height) * std::min(windowSize.x / width, windowSize.y / height);
	ImGui::SetCursorPos((windowSize - size) * 0.5);

	texture.BufferImage();

    auto draw_list = ImGui::GetWindowDrawList();

	draw_list->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerNearest);
	ImGui::Image(reinterpret_cast<void*>(texture.GetTextureId()), size);
	draw_list->AddCallback(ImGui::GetPlatformIO().DrawCallback_SetSamplerLinear);

	ImGui::End();
}

class ICore {
  public:
	virtual ~ICore() = default;

	virtual std::string GetName() = 0;
	virtual md5 GetRomHash() = 0;

	virtual std::vector<MemoryDomain> GetMemoryDomains() = 0;
	virtual void WriteMemory(int domain, size_t address, uint8_t val) = 0;
	virtual uint8_t ReadMemory(int domain, size_t address) = 0;

	virtual void Draw() = 0;
	virtual void DrawMenuBar(bool& menuOpen) = 0;

	virtual void SaveState(nlohmann::json& saver) const = 0;
	virtual void LoadState(const nlohmann::json& saver) = 0;

	virtual void LoadRom(const std::string& path) = 0;

	virtual void Reset() = 0;
	virtual void HardReset() = 0;
	virtual void Update() = 0;

    // used to calculate window size
    virtual ImVec2 GetSize() const = 0;
};
