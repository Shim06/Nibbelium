#ifndef MAIN_H
#define MAIN_H

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"
#include "json.hpp"
#include <SDL.h>
#include <sstream>
#include <stack>
#include <stdio.h>
#include <string>
#include <Windows.h>
#include <shellapi.h>
#include <commdlg.h>
#include <vector>

#include "Emulator.h"

std::string LoadROM();
void SetupImGuiStyle();
std::vector<std::string> GetCh8FileNames(const std::string& folderPath);

#endif
