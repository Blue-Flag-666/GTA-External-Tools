// pch.hpp

#pragma once

#include <d3d11.h>
#include <filesystem>
#include <functional>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <set>
#include <string>
#include <tchar.h>
#include <TlHelp32.h>

#include "framework.hpp"
#include "Resource.hpp"

#include "imgui/imgui_internal.h"

namespace BF
{
	namespace filesystem = std::filesystem;
	using std::exception;
	using std::function;
	using std::initializer_list;
	using std::make_pair;
	using std::nullopt;
	using std::optional;
	using std::pair;
	using std::set;
	using std::string;
	using std::string_view;
	using std::to_string;
	using std::vector;
	using std::wstring_view;

	constexpr wstring_view OverlayTitle = L"GTA External Tools";            // 标题

	constexpr auto EPS = 1e-8;

	inline UINT8 to_hex(const char c)
	{
		if ('0' <= c && c <= '9' || 'A' <= c && c <= 'F')
		{
			return c - '0';
		}
		return 0;
	}

	template <typename T> bool equal(T x, T y)
	{
		return x == y;
	}

	template <> inline bool equal(const float x, const float y)
	{
		return abs(static_cast <double>(x - y)) < EPS;
	}
}

using namespace BF;  // NOLINT(clang-diagnostic-header-hygiene)

namespace ImGui
{
	void Spacing(float width, float height =0);
	bool Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f);
}
