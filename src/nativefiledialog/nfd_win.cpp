#ifdef _WIN32
#include <wchar.h>
#include <stdio.h>
#include <assert.h>
#include <windows.h>
#include <shobjidl.h>

#include "nfd.h"

#ifdef __MINGW32__
// Explicitly setting NTDDI version, this is necessary for the MinGW compiler
#define NTDDI_VERSION NTDDI_VISTA
#define _WIN32_WINNT _WIN32_WINNT_VISTA
#endif

#define _CRTDBG_MAP_ALLOC

/* only locally define UNICODE in this compilation unit */
#ifndef UNICODE
#define UNICODE
#endif

using namespace NFD;

static BOOL COMIsInitialized(HRESULT coResult) {
	if(coResult == RPC_E_CHANGED_MODE) {
		// If COM was previously initialized with different init flags,
		// NFD still needs to operate. Eat this warning.
		return TRUE;
	}

	return SUCCEEDED(coResult);
}

static HRESULT COMInit() {
	return CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
}

static void COMUninit(HRESULT coResult) {
	// do not uninitialize if RPC_E_CHANGED_MODE occurred -- this
	// case does not refcount COM.
	if(SUCCEEDED(coResult))
		CoUninitialize();
}

static std::string WCharToChar(const wchar_t* inStr) {
	int inStrCharacterCount = static_cast<int>(wcslen(inStr));
	int bytesNeeded = WideCharToMultiByte(CP_UTF8, 0, inStr, inStrCharacterCount, nullptr, 0, nullptr, nullptr);
	assert(bytesNeeded);
	bytesNeeded += 1;

	auto chars = new char[bytesNeeded];

	WideCharToMultiByte(CP_UTF8, 0, inStr, -1, chars, bytesNeeded, nullptr, nullptr);

	std::string ret = chars;
	delete[] chars;
	return ret;
}

// allocs the space in outStr -- call free()
static void CopyNFDCharToWChar(const char* inStr, wchar_t** outStr) {
	int inStrByteCount = static_cast<int>(strlen(inStr));
	int charsNeeded = MultiByteToWideChar(CP_UTF8, 0, inStr, inStrByteCount, nullptr, 0);
	assert(charsNeeded);
	assert(!*outStr);
	charsNeeded += 1; // terminator

	*outStr = new wchar_t[charsNeeded];

	int ret = MultiByteToWideChar(CP_UTF8, 0, inStr, inStrByteCount, *outStr, charsNeeded);
	(*outStr)[charsNeeded - 1] = '\0';
}

/* ext is in format "jpg", no wildcards or separators */
static Result AppendExtensionToSpecBuf(const char* ext, char* specBuf, size_t specBufLen) {
	const char SEP[] = ";";
	assert(specBufLen > strlen(ext)+3);

	if(strlen(specBuf) > 0) {
		strncat_s(specBuf, specBufLen, SEP, specBufLen - strlen(specBuf) - 1);
		specBufLen += strlen(SEP);
	}

	char extWildcard[NFD_MAX_STRLEN];
	int bytesWritten = sprintf_s(extWildcard, NFD_MAX_STRLEN, "*.%s", ext);
	assert(bytesWritten == (int)(strlen(ext)+2));

	strncat_s(specBuf, specBufLen, extWildcard, specBufLen - strlen(specBuf) - 1);

	return Result::Okay;
}

static Result AddFiltersToDialog(IFileDialog* fileOpenDialog, const char* filterList) {
	const wchar_t WILDCARD[] = L"*.*";

	if(!filterList || strlen(filterList) == 0)
		return Result::Okay;

	// Count rows to alloc
	UINT filterCount = 1; /* guaranteed to have one filter on a correct, non-empty parse */
	const char* p_filterList;
	for(p_filterList = filterList; *p_filterList; ++p_filterList) {
		if(*p_filterList == ';')
			++filterCount;
	}

	assert(filterCount);
	if(!filterCount) {
		NFDi_SetError("Error parsing filters.");
		return Result::Error;
	}

	/* filterCount plus 1 because we hardcode the *.* wildcard after the while loop */
	COMDLG_FILTERSPEC* specList = new COMDLG_FILTERSPEC[filterCount + 1];
	if(!specList) {
		return Result::Error;
	}
	for(UINT i = 0; i < filterCount + 1; ++i) {
		specList[i].pszName = nullptr;
		specList[i].pszSpec = nullptr;
	}

	size_t specIdx = 0;
	p_filterList = filterList;
	char typebuf[NFD_MAX_STRLEN] = {0}; /* one per comma or semicolon */
	char* p_typebuf = typebuf;

	char specbuf[NFD_MAX_STRLEN] = {0}; /* one per semicolon */

	while(true) {
		if(NFDi_IsFilterSegmentChar(*p_filterList)) {
			/* append a type to the specbuf (pending filter) */
			AppendExtensionToSpecBuf(typebuf, specbuf, NFD_MAX_STRLEN);

			p_typebuf = typebuf;
			memset(typebuf, 0, sizeof(char) * NFD_MAX_STRLEN);
		}

		if(*p_filterList == ';' || *p_filterList == '\0') {
			/* end of filter -- add it to specList */

			CopyNFDCharToWChar(specbuf, (wchar_t**)&specList[specIdx].pszName);
			CopyNFDCharToWChar(specbuf, (wchar_t**)&specList[specIdx].pszSpec);

			memset(specbuf, 0, sizeof(char) * NFD_MAX_STRLEN);
			++specIdx;
			if(specIdx == filterCount)
				break;
		}

		if(!NFDi_IsFilterSegmentChar(*p_filterList)) {
			*p_typebuf = *p_filterList;
			++p_typebuf;
		}

		++p_filterList;
	}

	/* Add wildcard */
	specList[specIdx].pszSpec = WILDCARD;
	specList[specIdx].pszName = WILDCARD;

	fileOpenDialog->SetFileTypes(filterCount + 1, specList);

	/* free speclist */
	for(size_t i = 0; i < filterCount; ++i) {
		delete specList[i].pszSpec;
	}
	delete[] specList;

	return Result::Okay;
}

static Result AllocPathSet(IShellItemArray* shellItems, std::vector<std::string>& outPaths) {
	const char ERRORMSG[] = "Error allocating pathset.";

	assert(shellItems);

	// How many items in shellItems?
	DWORD numShellItems;
	HRESULT result = shellItems->GetCount(&numShellItems);
	if(!SUCCEEDED(result)) {
		NFDi_SetError(ERRORMSG);
		return Result::Error;
	}
	
	/* fill buf */
	for(DWORD i = 0; i < numShellItems; ++i) {
		IShellItem* shellItem;
		result = shellItems->GetItemAt(i, &shellItem);
		if(!SUCCEEDED(result)) {
			NFDi_SetError(ERRORMSG);
			return Result::Error;
		}

		// Confirm SFGAO_FILESYSTEM is true for this shellitem, or ignore it.
		SFGAOF attribs;
		result = shellItem->GetAttributes(SFGAO_FILESYSTEM, &attribs);
		if(!SUCCEEDED(result)) {
			NFDi_SetError(ERRORMSG);
			return Result::Error;
		}
		if(!(attribs & SFGAO_FILESYSTEM))
			continue;

		LPWSTR name;
		shellItem->GetDisplayName(SIGDN_FILESYSPATH, &name);

		std::string cstring = WCharToChar(name);
		outPaths.push_back(cstring);
	}

	return Result::Okay;
}

static Result SetDefaultPath(IFileDialog* dialog, const char* defaultPath) {
	if(!defaultPath || strlen(defaultPath) == 0)
		return Result::Okay;

	wchar_t* defaultPathW = {0};
	CopyNFDCharToWChar(defaultPath, &defaultPathW);

	IShellItem* folder;
	HRESULT result = SHCreateItemFromParsingName(defaultPathW, nullptr, IID_PPV_ARGS(&folder));

	// Valid non results.
	if(result == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) || result == HRESULT_FROM_WIN32(ERROR_INVALID_DRIVE)) {
		delete defaultPathW;
		return Result::Okay;
	}

	if(!SUCCEEDED(result)) {
		NFDi_SetError("Error creating ShellItem");
		delete defaultPathW;
		return Result::Error;
	}

	// Could also call SetDefaultFolder(), but this guarantees defaultPath -- more consistency across API.
	dialog->SetFolder(folder);

	delete defaultPathW;
	folder->Release();

	return Result::Okay;
}

/* public */
Result NFD::OpenDialog(const char* filterList, const char* defaultPath, std::string& outPath) {
	Result nfdResult = Result::Error;


	HRESULT coResult = COMInit();
	if(!COMIsInitialized(coResult)) {
		NFDi_SetError("Could not initialize COM.");
		return nfdResult;
	}

	// Create dialog
	IFileOpenDialog* fileOpenDialog(nullptr);
	HRESULT result =CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,IID_IFileOpenDialog, reinterpret_cast<void**>(&fileOpenDialog));

	if(!SUCCEEDED(result)) {
		NFDi_SetError("Could not create dialog.");
		goto end;
	}

	// Build the filter list
	if(AddFiltersToDialog(fileOpenDialog, filterList) != Result::Okay) {
		goto end;
	}

	// Set the default path
	if(SetDefaultPath(fileOpenDialog, defaultPath) != Result::Okay) {
		goto end;
	}

	// Show the dialog.
	result = fileOpenDialog->Show(nullptr);
	if(SUCCEEDED(result)) {
		// Get the file name
		IShellItem* shellItem(nullptr);
		result = fileOpenDialog->GetResult(&shellItem);
		if(!SUCCEEDED(result)) {
			NFDi_SetError("Could not get shell item from dialog.");
			goto end;
		}
		wchar_t* filePath(NULL);
		result = shellItem->GetDisplayName(SIGDN_FILESYSPATH, &filePath);
		if(!SUCCEEDED(result)) {
			NFDi_SetError("Could not get file path for selected.");
			shellItem->Release();
			goto end;
		}

		outPath = WCharToChar(filePath);
		CoTaskMemFree(filePath);

		nfdResult = Result::Okay;
		shellItem->Release();
	} else if(result == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
		nfdResult = Result::Cancel;
	} else {
		NFDi_SetError("File dialog box show failed.");
		nfdResult = Result::Error;
	}

end:
	if(fileOpenDialog)
		fileOpenDialog->Release();

	COMUninit(coResult);

	return nfdResult;
}

Result NFD::OpenDialogMultiple(const char* filterList, const char* defaultPath, std::vector<std::string>& outPaths) {
	Result nfdResult = Result::Error;


	HRESULT coResult = COMInit();
	if(!COMIsInitialized(coResult)) {
		NFDi_SetError("Could not initialize COM.");
		return nfdResult;
	}

	// Create dialog
	IFileOpenDialog* fileOpenDialog(nullptr);
	HRESULT result =CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL,IID_IFileOpenDialog, reinterpret_cast<void**>(&fileOpenDialog));

	if(!SUCCEEDED(result)) {
		fileOpenDialog = nullptr;
		NFDi_SetError("Could not create dialog.");
		goto end;
	}

	// Build the filter list
	if(AddFiltersToDialog(fileOpenDialog, filterList) != Result::Okay) {
		goto end;
	}

	// Set the default path
	if(SetDefaultPath(fileOpenDialog, defaultPath) != Result::Okay) {
		goto end;
	}

	// Set a flag for multiple options
	DWORD dwFlags;
	result = fileOpenDialog->GetOptions(&dwFlags);
	if(!SUCCEEDED(result)) {
		NFDi_SetError("Could not get options.");
		goto end;
	}
	result = fileOpenDialog->SetOptions(dwFlags | FOS_ALLOWMULTISELECT);
	if(!SUCCEEDED(result)) {
		NFDi_SetError("Could not set options.");
		goto end;
	}

	// Show the dialog.
	result = fileOpenDialog->Show(nullptr);
	if(SUCCEEDED(result)) {
		IShellItemArray* shellItems;
		result = fileOpenDialog->GetResults(&shellItems);
		if(!SUCCEEDED(result)) {
			NFDi_SetError("Could not get shell items.");
			goto end;
		}

		std::string str;

		if(AllocPathSet(shellItems, outPaths) == Result::Error) {
			shellItems->Release();
			goto end;
		}

		shellItems->Release();
		nfdResult = Result::Okay;
	} else if(result == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
		nfdResult = Result::Cancel;
	} else {
		NFDi_SetError("File dialog box show failed.");
		nfdResult = Result::Error;
	}

end:
	if(fileOpenDialog)
		fileOpenDialog->Release();

	COMUninit(coResult);

	return nfdResult;
}

Result NFD::SaveDialog(const char* filterList, const char* defaultPath, std::string& outPath) {
	Result nfdResult = Result::Error;

	HRESULT coResult = COMInit();
	if(!COMIsInitialized(coResult)) {
		NFDi_SetError("Could not initialize COM.");
		return nfdResult;
	}

	// Create dialog
	::IFileSaveDialog* fileSaveDialog(nullptr);
	HRESULT result =CoCreateInstance(CLSID_FileSaveDialog, nullptr, CLSCTX_ALL,IID_IFileSaveDialog, reinterpret_cast<void**>(&fileSaveDialog));

	if(!SUCCEEDED(result)) {
		fileSaveDialog = nullptr;
		NFDi_SetError("Could not create dialog.");
		goto end;
	}

	// Build the filter list
	if(AddFiltersToDialog(fileSaveDialog, filterList) != Result::Okay) {
		goto end;
	}

	// Set the default path
	if(SetDefaultPath(fileSaveDialog, defaultPath) != Result::Okay) {
		goto end;
	}

	// Show the dialog.
	result = fileSaveDialog->Show(nullptr);
	if(SUCCEEDED(result)) {
		// Get the file name
		::IShellItem* shellItem;
		result = fileSaveDialog->GetResult(&shellItem);
		if(!SUCCEEDED(result)) {
			NFDi_SetError("Could not get shell item from dialog.");
			goto end;
		}
		wchar_t* filePath(nullptr);
		result = shellItem->GetDisplayName(SIGDN_FILESYSPATH, &filePath);
		if(!SUCCEEDED(result)) {
			shellItem->Release();
			NFDi_SetError("Could not get file path for selected.");
			goto end;
		}

		outPath = WCharToChar(filePath);
		CoTaskMemFree(filePath);

		nfdResult = Result::Okay;
		shellItem->Release();
	} else if(result == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
		nfdResult = Result::Cancel;
	} else {
		NFDi_SetError("File dialog box show failed.");
		nfdResult = Result::Error;
	}

end:
	if(fileSaveDialog)
		fileSaveDialog->Release();

	COMUninit(coResult);

	return nfdResult;
}

Result NFD::PickFolder(const char* defaultPath, std::string& outPath) {
	Result nfdResult = Result::Error;
	DWORD dwOptions = 0;

	HRESULT coResult = COMInit();
	if(!COMIsInitialized(coResult)) {
		NFDi_SetError("CoInitializeEx failed.");
		return nfdResult;
	}

	// Create dialog
	IFileOpenDialog* fileDialog(nullptr);
	HRESULT result = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_PPV_ARGS(&fileDialog));
	if(!SUCCEEDED(result)) {
		NFDi_SetError("CoCreateInstance for CLSID_FileOpenDialog failed.");
		goto end;
	}

	// Set the default path
	if(SetDefaultPath(fileDialog, defaultPath) != Result::Okay) {
		NFDi_SetError("SetDefaultPath failed.");
		goto end;
	}

	// Get the dialogs options
	if(!SUCCEEDED(fileDialog->GetOptions(&dwOptions))) {
		NFDi_SetError("GetOptions for IFileDialog failed.");
		goto end;
	}

	// Add in FOS_PICKFOLDERS which hides files and only allows selection of folders
	if(!SUCCEEDED(fileDialog->SetOptions(dwOptions | FOS_PICKFOLDERS))) {
		NFDi_SetError("SetOptions for IFileDialog failed.");
		goto end;
	}

	// Show the dialog to the user
	result = fileDialog->Show(nullptr);
	if(SUCCEEDED(result)) {
		// Get the folder name
		::IShellItem* shellItem(nullptr);

		result = fileDialog->GetResult(&shellItem);
		if(!SUCCEEDED(result)) {
			NFDi_SetError("Could not get file path for selected.");
			shellItem->Release();
			goto end;
		}

		wchar_t* path = nullptr;
		result = shellItem->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &path);
		if(!SUCCEEDED(result)) {
			NFDi_SetError("GetDisplayName for IShellItem failed.");
			shellItem->Release();
			goto end;
		}

		outPath = WCharToChar(path);
		CoTaskMemFree(path);

		nfdResult = Result::Okay;
		shellItem->Release();
	} else if(result == HRESULT_FROM_WIN32(ERROR_CANCELLED)) {
		nfdResult = Result::Cancel;
	} else {
		NFDi_SetError("Show for IFileDialog failed.");
		nfdResult = Result::Error;
	}

end:

	if(fileDialog)
		fileDialog->Release();

	COMUninit(coResult);

	return nfdResult;
}
#endif
