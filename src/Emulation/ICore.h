#pragma once
#include <string>

#include "saver.h"
#include "RenderImage.h"
#include "md5.h"

struct MemoryDomain {
	int Id;
	std::string Name;
	size_t Size;
};

class ICore {
public:
	virtual std::string GetName() = 0;
	virtual RenderImage* GetMainTexture() = 0;

	virtual md5 GetRomHash() = 0;

	virtual std::vector<MemoryDomain> GetMemoryDomains() = 0;
	virtual void WriteMemory(int domain, size_t address, uint8_t val) = 0;
	virtual uint8_t ReadMemory(int domain, size_t address) = 0;
	
    virtual void DrawMenuBar(bool& menuOpen) = 0;
    virtual void DrawWindows() = 0;

	virtual void SaveState(saver& saver) = 0;
	virtual void LoadState(saver& saver) = 0;
	
    virtual void LoadRom(const std::string& path) = 0;

	virtual void Reset() = 0;
	virtual void HardReset() = 0;
    virtual void Update() = 0;
};
