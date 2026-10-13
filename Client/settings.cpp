#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#include <Windows.h>

#include "settings.h"

static json settings;
static HMODULE g_Module = nullptr;
static bool g_Loaded = false;

static std::string TrimString(const std::string &str) {
	const auto first = str.find_first_not_of(" \t\r\n");
	if (first == std::string::npos) {
		return "";
	}
	const auto last = str.find_last_not_of(" \t\r\n");
	return str.substr(first, last - first + 1);
}

static std::string ToLowerAscii(std::string str) {
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
		return static_cast<char>(std::tolower(c));
	});
	return str;
}

static std::string GetConfigPath() {
	char path[MAX_PATH] = { 0 };
	if (GetModuleFileNameA(g_Module, path, MAX_PATH)) {
		std::string fullPath(path);
		const auto slash = fullPath.find_last_of("\\/");
		const auto dot = fullPath.find_last_of('.');
		std::string dir = (slash != std::string::npos) ? fullPath.substr(0, slash + 1) : "";
		std::string stemCfg = (dot != std::string::npos && (slash == std::string::npos || dot > slash))
			? (fullPath.substr(0, dot) + ".cfg")
			: (fullPath + ".cfg");

		std::ifstream testStem(stemCfg);
		if (testStem.good()) {
			return stemCfg;
		}

		std::string defaultCfg = dir + "mmultiplayer.cfg";
		std::ifstream testDefault(defaultCfg);
		if (testDefault.good()) {
			return defaultCfg;
		}

		return stemCfg;
	}

	return "mmultiplayer.cfg";
}

static void InitDefaults() {
	settings = json::object();

	settings["display"] = {
		{ "fullscreen", 0 },
		{ "res", "0x0" },
	};

	settings["client"] = {
		{ "character", 0 },
		{ "chatKeybind", 84 },
		{ "disabled", false },
		{ "name", "anonymous" },
		{ "peerIp", "" },
		{ "room", "lobby" },
		{ "showChatOverlay", true },
		{ "showNameTags", true },
	};

	settings["menu"] = {
		{ "showKeybind", 45 },
	};

	settings["player"] = {
		{ "showInfo", false },
	};

	settings["trainer"] = {
		{ "beamerKeybind", 84 },
		{ "enabled", false },
		{ "flyDownKeybind", 16 },
		{ "flyFasterKeybind", 69 },
		{ "flyKeybind", 50 },
		{ "flySlowerKeybind", 81 },
		{ "flyUpKeybind", 32 },
		{ "godKeybind", 49 },
		{ "kgKeybind", 69 },
		{ "loadKeybind", 53 },
		{ "overlay", true },
		{ "saveKeybind", 52 },
		{ "strangKeybind", 82 },
	};
}

static bool ParseBoolValue(const std::string &raw, bool fallback) {
	const auto lower = ToLowerAscii(TrimString(raw));
	if (lower == "1" || lower == "true" || lower == "yes" || lower == "on") {
		return true;
	}
	if (lower == "0" || lower == "false" || lower == "no" || lower == "off") {
		return false;
	}
	return fallback;
}

static int ParseIntValue(const std::string &raw, int fallback) {
	const auto trimmed = TrimString(raw);
	if (trimmed.empty()) {
		return fallback;
	}
	const auto lower = ToLowerAscii(trimmed);
	if (lower == "true" || lower == "yes" || lower == "on") {
		return 1;
	}
	if (lower == "false" || lower == "no" || lower == "off") {
		return 0;
	}
	char *endPtr = nullptr;
	const long parsed = std::strtol(trimmed.c_str(), &endPtr, 0);
	if (endPtr == trimmed.c_str()) {
		return fallback;
	}
	return static_cast<int>(parsed);
}

static std::string ResolveCanonicalKey(const json &sectionObj, const std::string &rawKey) {
	if (!sectionObj.is_object()) {
		return rawKey;
	}
	const auto lowerKey = ToLowerAscii(rawKey);
	if (sectionObj.find("res") != sectionObj.end() && lowerKey == "resolution") {
		return "res";
	}
	for (auto it = sectionObj.begin(); it != sectionObj.end(); ++it) {
		if (ToLowerAscii(it.key()) == lowerKey) {
			return it.key();
		}
	}
	return rawKey;
}

void Settings::SetModule(HMODULE module) {
	g_Module = module;
}

bool Settings::LoadDisplayConfig(bool &fullscreen, int &width, int &height) {
	if (!g_Loaded) {
		Settings::Load();
	}

	int monitorW = GetSystemMetrics(SM_CXSCREEN);
	int monitorH = GetSystemMetrics(SM_CYSCREEN);
	if (monitorW < 320) monitorW = 1920;
	if (monitorH < 200) monitorH = 1080;

	fullscreen = false;
	width = monitorW;
	height = monitorH;

	if (settings["display"].is_object()) {
		const auto &fsVal = settings["display"]["fullscreen"];
		if (fsVal.is_boolean()) {
			fullscreen = fsVal.get<bool>();
		} else if (fsVal.is_number_integer()) {
			fullscreen = (fsVal.get<int>() != 0);
		}

		const auto &resVal = settings["display"]["res"];
		if (resVal.is_string()) {
			const auto resStr = ToLowerAscii(resVal.get<std::string>());
			int w = 0, h = 0;
			if (sscanf_s(resStr.c_str(), "%dx%d", &w, &h) == 2) {
				width = (w <= 0) ? monitorW : (w < 320 ? 320 : w);
				height = (h <= 0) ? monitorH : (h < 200 ? 200 : h);
			}
		}
	}

	return true;
}

void Settings::SaveDisplayConfig(bool fullscreen, int width, int height) {
	if (!g_Loaded) {
		Settings::Load();
	}

	if (!settings["display"].is_object()) {
		settings["display"] = json::object();
	}

	settings["display"]["fullscreen"] = fullscreen ? 1 : 0;
	settings["display"]["res"] = std::to_string(width) + "x" + std::to_string(height);
	Settings::Save();
}

void Settings::SetSetting(const char *menu, const char *key, json value) {
	if (!g_Loaded) {
		Settings::Load();
	}

	if (!settings[menu].is_object()) {
		settings[menu] = json::object();
	}

	settings[menu][key] = value;
	Settings::Save();
}

json Settings::GetSetting(const char *menu, const char *key, json defaultValue) {
	if (!g_Loaded) {
		Settings::Load();
	}

	if (!settings[menu].is_object()) {
		settings[menu] = json::object();
	}

	auto &v = settings[menu][key];
	if (v.is_null() || (v.type() != defaultValue.type() && v.is_number() != defaultValue.is_number())) {
		v = defaultValue;
		Settings::Save();
	}

	return v;
}

void Settings::Load() {
	InitDefaults();
	g_Loaded = true;

	const auto cfgPath = GetConfigPath();
	std::ifstream file(cfgPath);
	if (!file.good()) {
		Settings::Save();
		return;
	}

	std::string currentSection = "display";
	std::string line;
	while (std::getline(file, line)) {
		line = TrimString(line);
		if (line.empty() || line[0] == '#' || line[0] == ';') {
			continue;
		}

		if (line.front() == '[' && line.back() == ']' && line.size() > 2) {
			currentSection = ToLowerAscii(TrimString(line.substr(1, line.size() - 2)));
			if (!settings[currentSection].is_object()) {
				settings[currentSection] = json::object();
			}
			continue;
		}

		const auto eq = line.find('=');
		if (eq == std::string::npos) {
			continue;
		}

		const auto rawKey = TrimString(line.substr(0, eq));
		const auto rawVal = TrimString(line.substr(eq + 1));
		if (rawKey.empty()) {
			continue;
		}

		auto &secObj = settings[currentSection];
		const auto canonicalKey = ResolveCanonicalKey(secObj, rawKey);
		auto &existing = secObj[canonicalKey];

		if (existing.is_boolean()) {
			existing = ParseBoolValue(rawVal, existing.get<bool>());
		} else if (existing.is_number_integer()) {
			existing = ParseIntValue(rawVal, existing.get<int>());
		} else if (existing.is_string()) {
			existing = rawVal;
		} else {
			const auto lowerVal = ToLowerAscii(rawVal);
			if (lowerVal == "true" || lowerVal == "false") {
				existing = (lowerVal == "true");
			} else {
				char *endPtr = nullptr;
				const long parsed = std::strtol(rawVal.c_str(), &endPtr, 0);
				if (!rawVal.empty() && endPtr && *endPtr == '\0') {
					existing = static_cast<int>(parsed);
				} else {
					existing = rawVal;
				}
			}
		}
	}

	file.close();
	Settings::Save();
}

void Settings::Reset() {
	InitDefaults();
	g_Loaded = true;
	Settings::Save();
}

void Settings::Save() {
	const auto cfgPath = GetConfigPath();
	std::ofstream file(cfgPath, std::ios::out | std::ios::trunc);
	if (!file) {
		printf("settings: failed to save %s\n", cfgPath.c_str());
		return;
	}

	const std::vector<std::string> sectionOrder = {
		"display",
		"client",
		"menu",
		"player",
		"trainer",
	};

	auto writeSection = [&](const std::string &secName, const json &secObj, bool &firstSection) {
		if (!secObj.is_object() || secObj.empty()) {
			return;
		}

		if (!firstSection) {
			file << "\n";
		}
		firstSection = false;

		file << "[" << secName << "]\n";
		for (auto it = secObj.begin(); it != secObj.end(); ++it) {
			file << it.key() << "=";
			const auto &val = it.value();
			if (val.is_boolean()) {
				file << (val.get<bool>() ? "true" : "false");
			} else if (val.is_number_integer()) {
				file << val.get<int>();
			} else if (val.is_number()) {
				file << val.get<double>();
			} else if (val.is_string()) {
				file << val.get<std::string>();
			}
			file << "\n";
		}
	};

	bool firstSection = true;
	for (const auto &secName : sectionOrder) {
		if (settings.find(secName) != settings.end()) {
			writeSection(secName, settings[secName], firstSection);
		}
	}

	for (auto it = settings.begin(); it != settings.end(); ++it) {
		if (std::find(sectionOrder.begin(), sectionOrder.end(), it.key()) == sectionOrder.end()) {
			writeSection(it.key(), it.value(), firstSection);
		}
	}

	file.close();
}