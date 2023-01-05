// pch.cpp: 与预编译标头对应的源文件

#include "pch.hpp"

void ImGui::Spacing(const float width, const float height)
{
	if (width != 0)
	{
		SameLine();
	}
	if (GetCurrentWindow()->SkipItems)
	{
		return;
	}
	ItemSize(ImVec2(width, height));
}

bool ImGui::Splitter(const bool split_vertically, const float thickness, float* size1, float* size2, const float min_size1, const float min_size2, const float splitter_long_axis_size)
{
	const ImGuiContext& g      = *GImGui;
	ImGuiWindow*        window = g.CurrentWindow;
	const ImGuiID       id     = window->GetID("##Splitter");
	ImRect              bb;
	bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
	bb.Max = bb.Min + CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
	return SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f);
}
