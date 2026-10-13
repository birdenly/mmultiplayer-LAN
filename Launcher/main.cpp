#include "stdafx.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, char *, int) {
    if (!AdjustCurrentPrivilege(SE_DEBUG_NAME)) {
        MessageBox(0, L"Failed to adjust privileges to debug", L"Failure", MB_OK);
        return 1;
    }

    const auto thread = CreateDialogThread();
    for (;; Sleep(200)) {
        const auto pids = GetProcessIdsByName(L"mirrorsedge.exe");
        if (pids.empty()) {
            continue;
        }

        bool injectedAny = false;
        for (const auto pid : pids) {
            const auto process = OpenProcess(PROCESS_ALL_ACCESS, false, pid);
            if (!process) {
                continue;
            }

            if (HasClientModule(process)) {
                CloseHandle(process);
                continue;
            }

            if (!HasModule(process, L"openal32.dll")) {
                CloseHandle(process);
                continue;
            }

            TerminateThread(thread, 0);

            if (LoadClient(process)) {
                injectedAny = true;
            }
            CloseHandle(process);
        }

        if (injectedAny) {
            return 0;
        }
    }
}

bool LoadClient(HANDLE process) {
    std::wstring path;
    if (!GetDllPath(GetProcessId(process), path)) {
        MessageBox(0, L"Failed to find local Client.dll (or mmultiplayer.dll) next to Launcher.exe", L"Failure", 0);
        return false;
    }

    bool status = false;

    const auto size = (path.size() + 1) * sizeof(wchar_t);
    const auto arg =
        VirtualAllocEx(process, nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    if (arg) {
        if (WriteProcessMemory(process, arg, path.c_str(), size, nullptr)) {
            const auto thread =
                CreateRemoteThread(process, nullptr, 0,
                                   reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(
                                       GetModuleHandle(L"kernel32.dll"), "LoadLibraryW")),
                                   arg, 0, nullptr);

            if (thread) {
                WaitForSingleObject(thread, INFINITE);
                CloseHandle(thread);

                status = true;
            } else {
                MessageBox(0, L"Failed to create remote thread", L"Failure", 0);
            }
        } else {
            MessageBox(0, L"Failed to write process memory", L"Failure", 0);
        }

        VirtualFreeEx(process, arg, 0, MEM_RELEASE);
    } else {
        MessageBox(0, L"Failed to allocate virtual memory", L"Failure", 0);
    }

    return status;
}

HANDLE CreateDialogThread() {
    return CreateThread(
        nullptr, 0,
        [](void *) -> unsigned long {
            MessageBox(0, L"Waiting for Mirror's Edge to start. Click OK to stop.", L"Waiting...",
                       MB_OK);

            exit(0);
            return 0;
        },
        nullptr, 0, nullptr);
}

bool HasModule(HANDLE process, const wchar_t *module) {
    const auto snapshot =
        CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetProcessId(process));

    if (snapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    MODULEENTRY32 entry = {sizeof(entry)};
    if (Module32First(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szModule, module) == 0) {
                CloseHandle(snapshot);
                return true;
            }
        } while (Module32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return false;
}

bool HasClientModule(HANDLE process) {
    const auto snapshot =
        CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetProcessId(process));

    if (snapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    MODULEENTRY32 entry = {sizeof(entry)};
    if (Module32First(snapshot, &entry)) {
        do {
            if (_wcsnicmp(entry.szModule, L"mmultiplayer", 12) == 0 ||
                _wcsicmp(entry.szModule, L"Client.dll") == 0) {
                CloseHandle(snapshot);
                return true;
            }
        } while (Module32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return false;
}

bool GetDllPath(DWORD pid, std::wstring &path) {
    wchar_t exeDir[MAX_PATH] = {0};
    if (!GetModuleFileNameW(nullptr, exeDir, MAX_PATH)) {
        return false;
    }
    PathRemoveFileSpecW(exeDir);

    const std::wstring candidates[] = {
        std::wstring(exeDir) + L"\\Client.dll",
        std::wstring(exeDir) + L"\\mmultiplayer.dll",
        std::wstring(exeDir) + L"\\..\\Client\\binary\\Client.dll",
    };

    std::wstring sourceDll;
    for (const auto &candidate : candidates) {
        if (PathFileExistsW(candidate.c_str())) {
            sourceDll = candidate;
            break;
        }
    }

    wchar_t tempDir[0x200] = {0};
    if (GetTempPathW(sizeof(tempDir) / sizeof(tempDir[0]), tempDir)) {
        std::wstring tempDll = std::wstring(tempDir) + L"mmultiplayer.dll";
        std::wstring pidTempDll =
            std::wstring(tempDir) + L"mmultiplayer_" + std::to_wstring(pid) + L".dll";
        if (!sourceDll.empty()) {
            if (CopyFileW(sourceDll.c_str(), tempDll.c_str(), FALSE)) {
                path = tempDll;
                return true;
            }
            if (CopyFileW(sourceDll.c_str(), pidTempDll.c_str(), FALSE)) {
                path = pidTempDll;
                return true;
            }
            if (PathFileExistsW(tempDll.c_str())) {
                path = tempDll;
                return true;
            }
        } else if (PathFileExistsW(tempDll.c_str())) {
            path = tempDll;
            return true;
        }
    }

    if (!sourceDll.empty()) {
        path = sourceDll;
        return true;
    }

    return false;
}

std::vector<DWORD> GetProcessIdsByName(const wchar_t *name) {
    std::vector<DWORD> pids;
    const auto snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return pids;
    }

    PROCESSENTRY32 entry = {sizeof(entry)};
    if (Process32First(snapshot, &entry)) {
        do {
            if (_wcsicmp(entry.szExeFile, name) == 0 ||
                _wcsnicmp(entry.szExeFile, L"mirrorsedge", 11) == 0) {
                pids.push_back(entry.th32ProcessID);
            }
        } while (Process32Next(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return pids;
}

bool AdjustCurrentPrivilege(const wchar_t *privilege) {
    LUID luid;
    if (!LookupPrivilegeValue(nullptr, privilege, &luid)) {
        return FALSE;
    }

    TOKEN_PRIVILEGES tp = {0};
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    HANDLE token;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &token)) {
        return FALSE;
    }

    if (!AdjustTokenPrivileges(token, false, &tp, sizeof(tp), nullptr, nullptr)) {
        CloseHandle(token);
        return FALSE;
    }

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED) {
        CloseHandle(token);
        return FALSE;
    }

    CloseHandle(token);
    return TRUE;
}