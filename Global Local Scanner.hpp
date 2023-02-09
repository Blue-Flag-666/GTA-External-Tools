#pragma once

#include "pch.hpp"
#include "Memory.hpp"

namespace BF::Global_Local_Scanner
{
	void show(bool& show, bool& first);

	void ScanGlobalLocal(std::set <ScanResult>& s, int type, const ScanValue& value);

	template <typename T, ScanType ST> void ScanGlobalLocal(std::set <ScanResult>& s, T value)
	{
		if (s.empty())
		{
			// TODO
		}
		else
		{
			for (const auto& result : s)
			{
				if (result.type == ST)
				{
					if (equal(GTA5.get_global <T>(result.offset), value))
					{
						s.erase(result);
					}
				}
			}
		}
	}

	template <> inline void ScanGlobalLocal <const char*, SCAN_STRING>(std::set <ScanResult>& s, const char* value)
	{
		if (s.empty())
		{
			//TODO
			(void) s;
		}
		else
		{
			for (const auto& result : s)
			{
				if (result.type == SCAN_STRING)
				{
					if (!strcmp(GTA5.get_global <const char*>(result.offset), value))
					{
						s.erase(result);
					}
				}
			}
		}
	}
}
