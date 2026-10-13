#pragma once

#include <Windows.h>
#include "json.h"

namespace Settings {

void SetModule(HMODULE module);
bool LoadDisplayConfig(bool &fullscreen, int &width, int &height);
void SaveDisplayConfig(bool fullscreen, int width, int height);

void SetSetting(const char *sub, const char *key, json value);
json GetSetting(const char *sub, const char *key, json defaultValue);
void Load();
void Reset();
void Save();

} // namespace Settings