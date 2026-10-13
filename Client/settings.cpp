#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <string>
#include <Windows.h>

#include "settings.h"

static json settings;
static HMODULE g_Module = nullptr;

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

static std::string GetModuleDirectory() {
	char path[MAX_PATH] = { 0 };
	if (GetModuleFileNameA(g_Module, path, MAX_PATH)) {
		std::string fullPath(path);
		const auto pos = fullPath.find_last_of("\\/");
		if (pos != std::string::npos) {
			return fullPath.substr(0, pos + 1);
		}
	}
	return "";
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

static std::string GetSettingsPath() {
	return GetModuleDirectory() + "mmultiplayer.settings";
}

void Settings::SetModule(HMODULE module) {
	g_Module = module;
}

bool Settings::LoadDisplayConfig(bool &fullscreen, int &width, int &height) {
	fullscreen = false;
	width = 1920;
	height = 540;

	const auto cfgPath = GetConfigPath();
	std::ifstream file(cfgPath);
	if (!file.good()) {
		SaveDisplayConfig(fullscreen, width, height);
		return true;
	}

	bool foundAny = false;
	std::string line;
	while (std::getline(file, line)) {
		line = TrimString(line);
		if (line.empty() || line[0] == '#' || line[0] == ';' || line[0] == '[') {
			continue;
		}

		const auto eq = line.find('=');
		if (eq == std::string::npos) {
			continue;
		}

		const auto key = ToLowerAscii(TrimString(line.substr(0, eq)));
		const auto val = ToLowerAscii(TrimString(line.substr(eq + 1)));

		if (key == "fullscreen") {
			fullscreen = (val == "1" || val == "true" || val == "yes");
			foundAny = true;
		} else if (key == "res" || key == "resolution") {
			int w = 0, h = 0;
			if (sscanf_s(val.c_str(), "%dx%d", &w, &h) == 2 && w >= 320 && h >= 200) {
				width = w;
				height = h;
				foundAny = true;
			}
		}
	}

	return foundAny;
}

void Settings::SaveDisplayConfig(bool fullscreen, int width, int height) {
	const auto cfgPath = GetConfigPath();
	std::ofstream file(cfgPath, std::ios::out | std::ios::trunc);
	if (!file) {
		return;
	}

	file << "fullscreen=" << (fullscreen ? 1 : 0) << "\n";
	file << "res=" << width << "x" << height << "\n";
}

void Settings::SetSetting(const char *menu, const char *key, json value) {
	if (settings[menu].is_null()) {
		settings[menu] = json::object();
	}

	settings[menu][key] = value;
	Settings::Save();
}

json Settings::GetSetting(const char *menu, const char *key, json defaultValue) {
	if (settings[menu].is_null()) {
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
	auto reset = true;

	std::ifstream f(GetSettingsPath());
	if (f.good()) {
		try {
			settings = json::parse(f);
			reset = false;
		} catch (json::parse_error &) {}

		f.close();
	}

	if (reset) {
		Reset();
	}
}

void Settings::Reset() {
	settings = json::object();
	Settings::Save();
}

void Settings::Save() {
	std::ofstream file(GetSettingsPath(), std::ios::out);
	if (!file) {
		printf("settings: failed to save %s\n", GetSettingsPath().c_str());
		return;
	}

	auto dump = settings.dump();
	file.write(dump.c_str(), dump.size());
	file.close();
}