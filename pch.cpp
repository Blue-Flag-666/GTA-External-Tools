#include "pch.hpp"

namespace BF
{
	UINT8 to_hex(const char c)
	{
		if ('0' <= c && c <= '9' || 'A' <= c && c <= 'F')
		{
			return c - '0';
		}
		return 0;
	}

	void AllocCon()
	{
		AllocConsole();
		FILE* stream;
		freopen_s(&stream, "CON", "r", stdin);		// NOLINT(cert-err33-c)
		freopen_s(&stream, "CON", "w", stdout);		// NOLINT(cert-err33-c)
		freopen_s(&stream, "CON", "w", stderr);		// NOLINT(cert-err33-c)
		if (const auto consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE); consoleHandle)
		{
			SetConsoleCP(CP_UTF8);
			SetConsoleOutputCP(CP_UTF8);

			DWORD consoleMode;
			GetConsoleMode(consoleHandle, &consoleMode);

			consoleMode = consoleMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | DISABLE_NEWLINE_AUTO_RETURN;
			consoleMode = consoleMode & ~ENABLE_QUICK_EDIT_MODE;

			SetConsoleMode(consoleHandle, consoleMode);
		}
	}

	void ClearConsole()
	{
		const HANDLE               hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO csbi;

		if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
		{
			return;
		}

		const SMALL_RECT scrollRect { 0, 0, csbi.dwSize.X, csbi.dwSize.Y };
		const COORD      scrollTarget { 0, static_cast <short>(-csbi.dwSize.Y) };
		const CHAR_INFO  fill { ' ', csbi.wAttributes };

		ScrollConsoleScreenBuffer(hConsole, &scrollRect, nullptr, scrollTarget, &fill);

		csbi.dwCursorPosition.X = 0;
		csbi.dwCursorPosition.Y = 0;

		SetConsoleCursorPosition(hConsole, csbi.dwCursorPosition);
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
