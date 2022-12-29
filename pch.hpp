// pch.hpp

#pragma once

#include <d3d11.h>
#include <filesystem>
#include <functional>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <string>
#include <tchar.h>
#include <TlHelp32.h>

#include "framework.hpp"
#include "Resource.hpp"

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
	using std::string;
	using std::string_view;
	using std::vector;
	using std::wstring_view;

	constexpr wstring_view OverlayTitle = L"GTA External Tools";            // 标题

	inline UINT8 to_hex(const char c)
	{
		if ('0' <= c && c <= '9' || 'A' <= c && c <= 'F')
		{
			return c - '0';
		}
		return 0;
	}
}

using namespace BF;  // NOLINT(clang-diagnostic-header-hygiene)
