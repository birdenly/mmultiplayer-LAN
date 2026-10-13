#pragma once

#include <Windows.h>
#include <shlwapi.h>
#include <stdio.h>
#include <tlhelp32.h>

#pragma comment(lib, "shlwapi.lib")

#include <string>
#include <thread>
#include <vector>

bool LoadClient(HANDLE process);
HANDLE CreateDialogThread();
bool HasModule(HANDLE process, const wchar_t *module);
bool HasClientModule(HANDLE process);
bool GetDllPath(std::wstring &path);
std::vector<DWORD> GetProcessIdsByName(const wchar_t *name);
bool AdjustCurrentPrivilege(const wchar_t *privilege);