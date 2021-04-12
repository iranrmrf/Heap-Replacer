#include "custom_widgets.h"

struct ImGuiGridArrayGetterData
{
    const ImU32* Values;
    int Stride;

    ImGuiGridArrayGetterData(const ImU32* values, int stride) { Values = values; Stride = stride; }
};

static ImU32 Grid_ArrayGetter(void* data, int idx)
{
    ImGuiGridArrayGetterData* plot_data = (ImGuiGridArrayGetterData*)data;
    const ImU32 v = *(const ImU32*)(const void*)((const unsigned char*)plot_data->Values + (size_t)idx * plot_data->Stride);
    return v;
}

void ImGui::PlotColorGrid(const char* label, const ImU32* values, int colors_count, int colors_offset, ImVec2 size, int stride)
{
    ImGui::PlotColorGrid(label, &Grid_ArrayGetter, (void*)&values, colors_count, colors_offset, size);
}

void ImGui::PlotColorGrid(const char* label, ImU32(*values_getter)(void* data, int idx), void* data, int colors_count, int colors_offset, ImVec2 size)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems) { return; }

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    const ImGuiID id = window->GetID(label);

    ImVec2 min = GetWindowContentRegionMin();
    ImVec2 max = GetWindowContentRegionMax();

    if (size.x == 0.0f) { size.x = max.x - min.x; }
    if (size.y == 0.0f) { size.y = max.y - min.y; }

    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + size);
    const ImRect inner_bb(frame_bb.Min + style.FramePadding, frame_bb.Max - style.FramePadding);

    ItemSize(frame_bb, style.FramePadding.y);
    if (!ItemAdd(frame_bb, 0, &inner_bb)) { return; }

    RenderFrame(frame_bb.Min, frame_bb.Max, GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

    size_t blocks_per_row = 0u;
    while (inner_bb.GetWidth() / inner_bb.GetHeight() > ++blocks_per_row / ceilf((float)colors_count / blocks_per_row));
    float box_size = inner_bb.GetWidth() / blocks_per_row;
    size_t blocks_per_col = (size_t)ceilf((float)colors_count / blocks_per_row);

    for (int i = 0; i < colors_count; i++)
    {
        div_t divr = div(i, blocks_per_row);
        ImVec2 pos0(inner_bb.Min.x + divr.rem * box_size, inner_bb.Min.y + divr.quot * box_size);
        ImVec2 pos1(pos0.x + box_size, pos0.y + box_size);
        window->DrawList->AddRectFilled(pos0, pos1, values_getter(data, (i + colors_offset) % colors_count));
    }
}
