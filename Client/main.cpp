#include <windows.h>

#include "debug.h"
#include "menu.h"
#include "settings.h"
#include "addon.h"

#include "addons/dolly.h"
#include "addons/client.h"
#include "addons/trainer.h"

#pragma comment(linker, "/EXPORT:DirectInput8Create=_DirectInput8Create@20")
extern "C" HRESULT WINAPI DirectInput8Create(HINSTANCE hinst, DWORD dwVersion,
                                             const IID &riidltf, LPVOID *ppvOut,
                                             LPUNKNOWN punkOuter) {
	static HMODULE realDinput8 = nullptr;
	typedef HRESULT(WINAPI *DirectInput8CreateFn)(HINSTANCE, DWORD, const IID &,
	                                              LPVOID *, LPUNKNOWN);
	static DirectInput8CreateFn realFunc = nullptr;

	if (!realFunc) {
		wchar_t sysDir[MAX_PATH] = { 0 };
		GetSystemDirectoryW(sysDir, MAX_PATH);
		wcscat_s(sysDir, L"\\dinput8.dll");
		realDinput8 = LoadLibraryW(sysDir);
		if (realDinput8) {
			realFunc = reinterpret_cast<DirectInput8CreateFn>(
				GetProcAddress(realDinput8, "DirectInput8Create"));
		}
	}

	return realFunc ? realFunc(hinst, dwVersion, riidltf, ppvOut, punkOuter)
	                : E_FAIL;
}

static bool IsMirrorsEdgeProcess() {
	if (GetModuleHandleW(L"MirrorsEdge.exe")) {
		return true;
	}

	wchar_t exePath[MAX_PATH] = { 0 };
	if (GetModuleFileNameW(nullptr, exePath, MAX_PATH)) {
		const wchar_t *fileName = wcsrchr(exePath, L'\\');
		fileName = fileName ? (fileName + 1) : exePath;
		if (_wcsnicmp(fileName, L"MirrorsEdge", 11) == 0) {
			return true;
		}
	}

	return false;
}

static DWORD WINAPI InitializeModThread(LPVOID) {
	while (!GetModuleHandleW(L"d3d9.dll") || !GetModuleHandleW(L"openal32.dll")) {
		Sleep(50);
	}

	Debug::Initialize();
	Settings::Load();

	Addon *addons[] = { new Client(), new Trainer(), new Dolly() };

	if (!Engine::Initialize()) {
		MessageBoxA(0, "Failed to initialize engine", "Fatal", 0);
		return 0;
	}

	if (!Menu::Initialize()) {
		MessageBoxA(0, "Failed to initialize menu", "Fatal", 0);
		return 0;
	}

	for (auto &addon : addons) {
		if (!addon->Initialize()) {
			MessageBoxA(0, ("Failed to initialize \"" + addon->GetName() + "\"").c_str(), "Fatal", 0);
		}
	}

	return 0;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
	if (reason == DLL_PROCESS_ATTACH && IsMirrorsEdgeProcess()) {
		static bool initialized = false;
		if (initialized) {
			return TRUE;
		}
		initialized = true;

		DisableThreadLibraryCalls(module);
		Settings::SetModule(module);

		bool fullscreen = false;
		int width = 1920;
		int height = 540;
		if (Settings::LoadDisplayConfig(fullscreen, width, height)) {
			Engine::SetDisplayConfig(fullscreen, width, height, false);
		}

		Engine::InstallStartupHooks();

		HANDLE thread = CreateThread(nullptr, 0, InitializeModThread, module, 0, nullptr);
		if (thread) {
			CloseHandle(thread);
		}
	}

	return TRUE;
}