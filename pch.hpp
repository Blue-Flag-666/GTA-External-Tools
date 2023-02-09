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
	constexpr std::wstring_view OverlayTitle = L"GTA External Tools";            // 标题
	constexpr auto              EPS          = 1e-8;
	constexpr auto              MAX_STR_LEN  = 0x100;
	constexpr auto              MAX_NAME_LEN = 0x20;

	template <typename T> bool equal(T x, T y)
	{
		return x == y;
	}

	template <> inline bool equal(const float x, const float y)
	{
		return abs(static_cast <double>(x) - static_cast <double>(y)) < EPS;
	}

	uint8_t to_hex(char c);

	inline void* int64_to_void(const uintptr_t value)
	{
		return reinterpret_cast <void*>(value);  // NOLINT(performance-no-int-to-ptr)
	}

	inline uintptr_t void_to_int64(const void* value)
	{
		return reinterpret_cast <uintptr_t>(value);
	}

	void AllocCon();
	void ClearConsole();

	enum ScanType
	{
		SCAN_BOOL   = 0b1,
		SCAN_FLOAT  = 0b10,
		SCAN_INT    = 0b100,
		SCAN_UINT   = 0b1000,
		SCAN_STRING = 0b10000,
		SCAN_NUMBER = 0b1110,
		SCAN_NONE   = 0
	};

	std::string to_string(ScanType type);

	struct ScanValue
	{
		bool     bool_value                = true;
		float    float_value               = 0;
		int      int_value                 = 0;
		uint32_t uint_value                = 0;
		char     string_value[MAX_STR_LEN] = "";

		ScanValue() = default;

		explicit ScanValue(const bool value): bool_value(value) {}

		explicit ScanValue(const float value): float_value(value),
											   int_value(static_cast <int>(value)),
											   uint_value(static_cast <uint32_t>(value)) {}

		explicit ScanValue(const double value): float_value(static_cast <float>(value)),
												int_value(static_cast <int>(value)),
												uint_value(static_cast <uint32_t>(value)) {}

		explicit ScanValue(const int value): int_value(value),
											 uint_value(value) {}

		explicit ScanValue(const uint32_t value): uint_value(value) {}

		explicit ScanValue(const char* value)
		{
			strcpy_s(string_value, value);
		}

		void reset(const char* value)
		{
			if (string_value == value)
			{
				bool_value  = true;
				float_value = 0;
				int_value   = 0;
				uint_value  = 0;
			}
			else
			{
				*this = ScanValue(value);
			}
		}
	};

	struct ScanResult
	{
		int       offset = 0;
		ScanType  type   = SCAN_INT;
		ScanValue prev;

		ScanResult(const int offset, const ScanType type) : offset(offset),
															type(type) {}

		bool operator<(const ScanResult& x) const
		{
			return offset < x.offset || (offset == x.offset && type < x.type);
		}
	};

	struct ScanItem
	{
		char     name[MAX_NAME_LEN] = "Unnamed";
		int      offset             = 0;
		ScanType type               = SCAN_INT;

		ScanItem(const int offset, const ScanType type) : offset(offset),
														  type(type) {}
	};
}

namespace ImGui
{
	void Spacing(float height);
	void Spacing(float width, float height);
	bool Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f);
}
