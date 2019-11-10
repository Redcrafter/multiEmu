#pragma once
#include <string>
#include <vector>

#define NFD_MAX_STRLEN 256

namespace NFD {
	enum class Result {
		Error, /* programmatic error */
		Okay,  /* user pressed okay, or successful return */
		Cancel /* user pressed cancel */
	};
	
	/* single file open dialog */
	Result OpenDialog(const char* filterList, const char* defaultPath, std::string& outPath);

	/* multiple file open dialog */
	Result OpenDialogMultiple(const char* filterList, const char* defaultPath, std::vector<std::string>& outPaths);

	/* save dialog */
	Result SaveDialog(const char* filterList, const char* defaultPath, std::string& outPath);

	/* select folder dialog */
	Result PickFolder(const char* defaultPath, std::string& outPath);

	/* get last error -- set when Result is NFD_ERROR */
	std::string GetError();
}

void NFDi_SetError(const char* msg);
int NFDi_IsFilterSegmentChar(char ch);