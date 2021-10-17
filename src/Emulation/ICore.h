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

class ICore {
  public:
	virtual ~ICore() = default;

	virtual std::string GetName() = 0;
	virtual RenderImage* GetMainTexture() = 0;
	virtual float GetPixelRatio() = 0;

	virtual md5 GetRomHash() = 0;

	virtual std::vector<MemoryDomain> GetMemoryDomains() = 0;
	virtual void WriteMemory(int domain, size_t address, uint8_t val) = 0;
	virtual uint8_t ReadMemory(int domain, size_t address) = 0;

	virtual void DrawMenuBar(bool& menuOpen) = 0;
	virtual void DrawWindows() = 0;

	virtual void DrawMainWindow() {
		auto windowSize = ImGui::GetWindowSize();
		auto texture = GetMainTexture();

		auto width = texture->GetWidth();
		auto height = texture->GetHeight();

		auto ratio = height / (width * GetPixelRatio());

		auto size = ImGui::GetWindowSize();
		if(size.x * ratio < size.y) {
			size.y = size.x * ratio;
		} else {
			size.x = size.y * (1 / ratio);
		}

		ImGui::SetCursorPos((windowSize - size) * 0.5);

		texture->BufferImage();
		ImGui::Image(reinterpret_cast<void*>(texture->GetTextureId()), size);
	}

	virtual void SaveState(saver& saver) = 0;
	virtual void LoadState(saver& saver) = 0;

	virtual void LoadRom(const std::string& path) = 0;

	virtual void Reset() = 0;
	virtual void HardReset() = 0;
	virtual void Update() = 0;
};
