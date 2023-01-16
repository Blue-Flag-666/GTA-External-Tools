#pragma once

#include <array>
#include <d3d11.h>
#include <filesystem>
#include <functional>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <imgui_internal.h>
#include <ranges>
#include <set>
#include <string>
#include <tchar.h>
#include <TlHelp32.h>
#include <unordered_set>

#include "framework.hpp"
#include "resource.h"

namespace BF
{
	constexpr std::wstring_view overlay_title = L"GTA External Tools";            // 标题
	constexpr auto              EPS           = 1e-8;
	constexpr auto              MAX_STR_LEN   = 0x100;
	constexpr auto              MAX_NAME_LEN  = 0x20;

	template <typename T> bool equal(T x, T y)
	{
		return x == y;
	}

	template <> inline bool equal(const float x, const float y)
	{
		return abs(static_cast <double>(x) - static_cast <double>(y)) < EPS;
	}

	uint8_t to_hex(char c);

	void alloc_console();
	void clear_console();

	enum scan_type
	{
		SCAN_BOOL   = 0b1,
		SCAN_FLOAT  = 0b10,
		SCAN_INT    = 0b100,
		SCAN_UINT   = 0b1000,
		SCAN_STRING = 0b10000,
		SCAN_NUMBER = 0b1110,
		SCAN_NONE   = 0
	};

	std::string to_string(scan_type type);

	struct scan_value
	{
		bool     bool_value                = true;
		float    float_value               = 0;
		int      int_value                 = 0;
		uint32_t uint_value                = 0;
		char     string_value[MAX_STR_LEN] = "";

		scan_value() = default;

		explicit scan_value(const bool value): bool_value(value) {}

		explicit scan_value(const float value): float_value(value),
												int_value(static_cast <int>(value)),
												uint_value(static_cast <uint32_t>(value)) {}

		explicit scan_value(const double value): float_value(static_cast <float>(value)),
												 int_value(static_cast <int>(value)),
												 uint_value(static_cast <uint32_t>(value)) {}

		explicit scan_value(const int value): int_value(value),
											  uint_value(value) {}

		explicit scan_value(const uint32_t value): uint_value(value) {}

		explicit scan_value(const char* value)
		{
			strcpy_s(string_value, value);
		}

		void reset(const char* value)
		{
			if (value == string_value)
			{
				bool_value  = true;
				float_value = 0;
				int_value   = 0;
				uint_value  = 0;
			}
			else
			{
				*this = scan_value(value);
			}
		}
	};

	struct scan_result
	{
		int        offset = 0;
		scan_type  type   = SCAN_INT;
		scan_value previous_value;

		scan_result(const int offset, const scan_type type) : offset(offset),
															  type(type) {}

		bool operator<(const scan_result& x) const
		{
			return offset < x.offset || (offset == x.offset && type < x.type);
		}
	};

	struct scan_item
	{
		char      name[MAX_NAME_LEN] = "Unnamed";
		int       offset             = 0;
		scan_type type               = SCAN_INT;

		scan_item(const int offset, const scan_type type) : offset(offset),
															type(type) {}
	};
}

namespace ImGui
{
	void Spacing(float height);
	void Spacing(float width, float height);
	bool Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f);
}
