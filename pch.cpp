#include "pch.hpp"

namespace BF
{
	uint8_t hex_to_dec(const char c)
	{
		if ('0' <= c && c <= '9')
		{
			return c - '0';
		}
		if ('A' <= c && c <= 'F')
		{
			return c - 'A' + 10;
		}
		if ('a' <= c && c <= 'f')
		{
			return c - 'a' + 10;
		}
		return 0;
	}

	void AllocCon()
	{
		AllocConsole();
		FILE* stream;
		if (freopen_s(&stream, "CON", "r", stdin) || freopen_s(&stream, "CON", "w", stdout) || freopen_s(&stream, "CON", "w", stderr))
		{
			MessageBox(nullptr, L"打开控制台失败", OverlayTitle.data(),MB_OK);
			throw std::exception("打开控制台失败");
		}
		if (const auto handle = GetStdHandle(STD_OUTPUT_HANDLE); handle)
		{
			SetConsoleCP(CP_UTF8);
			SetConsoleOutputCP(CP_UTF8);

			SetConsoleTitle(OverlayTitle.data());

			DWORD mode;
			GetConsoleMode(handle, &mode);

			mode = mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
			mode = mode & ~ENABLE_QUICK_EDIT_MODE;

			SetConsoleMode(handle, mode);
		}
	}

	void ClearConsole()
	{
		const HANDLE               handle = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO csbi;

		if (!GetConsoleScreenBufferInfo(handle, &csbi))
		{
			return;
		}

		const SMALL_RECT rect { 0, 0, csbi.dwSize.X, csbi.dwSize.Y };
		const COORD      scroll { 0, static_cast <short>(-csbi.dwSize.Y) };
		const CHAR_INFO  fill { { ' ' }, csbi.wAttributes };

		ScrollConsoleScreenBuffer(handle, &rect, nullptr, scroll, &fill);

		csbi.dwCursorPosition.X = 0;
		csbi.dwCursorPosition.Y = 0;

		SetConsoleCursorPosition(handle, csbi.dwCursorPosition);
	}

	std::string to_string(const ScanType type)
	{
		switch (type)
		{
			case SCAN_BOOL:
			{
				return "Bool";
			}
			case SCAN_FLOAT:
			{
				return "Float";
			}
			case SCAN_INT:
			{
				return "Int";
			}
			case SCAN_UINT:
			{
				return "UInt";
			}
			case SCAN_STRING:
			{
				return "String";
			}
			case SCAN_NUMBER:
			{
				return "Number";
			}
			case SCAN_NONE:
			{
				return "None";
			}
		}
		return "Unknown";
	}
}

namespace ImGui
{
	void Spacing(const float height)
	{
		if (GetCurrentWindow()->SkipItems)
		{
			return;
		}
		ItemSize(ImVec2(0, height));
	}

	void Spacing(const float width, const float height)
	{
		if (GetCurrentWindow()->SkipItems)
		{
			return;
		}
		ItemSize(ImVec2(width, height));
	}

	bool Splitter(const bool split_vertically, const float thickness, float* size1, float* size2, const float min_size1, const float min_size2, const float splitter_long_axis_size)
	{
		const ImGuiContext& g      = *GImGui;
		ImGuiWindow*        window = g.CurrentWindow;
		const ImGuiID       id     = window->GetID("##Splitter");
		ImRect              bb;
		bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
		bb.Max = bb.Min + CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
		return SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f);
	}
}
