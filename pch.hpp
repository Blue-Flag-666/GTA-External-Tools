#pragma once

#include <d3d11.h>
#include <filesystem>
#include <functional>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <ranges>
#include <set>
#include <string>
#include <tchar.h>
#include <TlHelp32.h>
#include <unordered_set>

#include "framework.hpp"
#include "resource.h"

#include "imgui/imgui_internal.h"

namespace BF
{
	namespace filesystem = std::filesystem;

	using std::exception;
	using std::for_each;
	using std::function;
	using std::initializer_list;
	using std::make_pair;
	using std::nullopt;
	using std::optional;
	using std::pair;
	using std::ranges::reverse_view;
	using std::set;
	using std::string;
	using std::string_view;
	using std::swap;
	using std::to_string;
	using std::unordered_set;
	using std::vector;
	using std::wstring_view;

	constexpr wstring_view OverlayTitle = L"GTA External Tools";            // 标题
	constexpr auto         EPS          = 1e-8;
	constexpr auto         MAX_STR_LEN  = 0x100;
	constexpr auto         MAX_NAME_LEN = 0x20;

	template <typename T> bool equal(T x, T y)
	{
		return x == y;
	}

	template <> inline bool equal(const float x, const float y)
	{
		return abs(static_cast <double>(x) - static_cast <double>(y)) < EPS;
	}

	UINT8 to_hex(char c);

	void AllocCon();
	void ClearConsole();
}

using namespace BF;  // NOLINT(clang-diagnostic-header-hygiene)

namespace ImGui
{
	void Spacing(float height);
	void Spacing(float width, float height);
	bool Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f);
}
