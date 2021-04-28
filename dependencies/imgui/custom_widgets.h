#pragma once

#define IMGUI_DEFINE_MATH_OPERATORS

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

namespace ImGui
{

	IMGUI_API void PlotColorGrid(const char* label, const ImU32* values, int colors_count, int colors_offset, ImVec2 size, int stride = sizeof(ImU32));
	IMGUI_API void PlotColorGrid(const char* label, ImU32(*values_getter)(void* data, int idx), void* data, int colors_count, int colors_offset = 0, ImVec2 size = ImVec2(0, 0));

};
