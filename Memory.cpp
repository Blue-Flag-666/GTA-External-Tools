#include "pch.hpp"
#include "Memory.hpp"
#include "GTA External Tools.hpp"

HWND GetProcessMainWnd(const DWORD dwProcessId)
{
	const WNDINFO             wndInfo { nullptr, dwProcessId };
	EnumWindows([](const HWND hWnd, const LPARAM lParam) ->BOOL
	{
		DWORD ProcessId = 0;
		GetWindowThreadProcessId(hWnd, &ProcessId);
		if (const auto pInfo = static_cast <WNDINFO*>(BF::int64_to_void(lParam)); ProcessId == pInfo->dwProcessId)
		{
			pInfo->hWnd = hWnd;
			return FALSE;
		}
		return TRUE;
	}, static_cast <LPARAM>(BF::void_to_int64(&wndInfo)));

	return wndInfo.hWnd;
}

BOOL ListSystemProcesses(WCHAR szExeFile[MAX_PATH], const LPPROCESSENTRY32 PE32)
{
	const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (snapshot == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	PE32->dwSize = sizeof(PROCESSENTRY32);
	if (!Process32First(snapshot, PE32))
	{
		CloseHandle(snapshot);
		return FALSE;
	}
	do
	{
		if (lstrcmp(szExeFile, PE32->szExeFile) == 0)
		{
			CloseHandle(snapshot);
			return TRUE;
		}
	}
	while (Process32Next(snapshot, PE32));
	CloseHandle(snapshot);
	return FALSE;
}

BOOL ListProcessModules(const uint32_t dwProcessId, const WCHAR szModule[MAX_MODULE_NAME32 + 1], const LPMODULEENTRY32 ME32)
{
	const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, dwProcessId);
	if (snapshot == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	ME32->dwSize = sizeof(MODULEENTRY32);
	if (!Module32First(snapshot, ME32))
	{
		CloseHandle(snapshot);
		return FALSE;
	}
	do
	{
		if (lstrcmp(szModule, ME32->szModule) == 0)
		{
			CloseHandle(snapshot);
			return TRUE;
		}
	}
	while (Module32Next(snapshot, ME32));
	CloseHandle(snapshot);
	return FALSE;
}

namespace BF
{
	uintptr_t Memory::Pointers::Pattern::AOBScan(const std::string_view pattern)
	{
		std::vector <std::optional <uint8_t>> compiled;

		uint8_t hexchar = 0;
		auto    first   = true, lastwaswc = false;

		for (const char c : pattern)
		{
			if (c == '?' && !lastwaswc)
			{
				lastwaswc = true;
				compiled.emplace_back(std::nullopt);
			}
			else
			{
				lastwaswc = false;
				if (c == ' ')
				{
					continue;
				}
				hexchar = hexchar + (first ? hex_to_dec(c) * 0x10 : hex_to_dec(c));

				if (!first)
				{
					compiled.emplace_back(hexchar);
					hexchar = 0;
				}
				first = !first;
			}
		}

		size_t i = GTA5.base_address;

		uintptr_t ret = 0;

		while (!ret)
		{
			constexpr auto CHUNK_SIZE = 0x1000;
			if (i >= GTA5.base_address + GTA5.process_size)
			{
				break;
			}

			if (uint8_t membuf[CHUNK_SIZE]; GTA5.read_mem <uint8_t*>(i, membuf, CHUNK_SIZE))
			{
				for (auto j = 0; j < CHUNK_SIZE; j++)
				{
					auto correct = true;
					for (size_t k = 0; k < compiled.size(); k++)
					{
						if (const auto& x = compiled[k]; x.has_value())
						{
							if (x.value() != membuf[j + k])
							{
								correct = false;
								break;
							}
						}
					}

					if (correct)
					{
						ret = i + j;
						break;
					}
				}
			}
			i = i + CHUNK_SIZE;
		}

		if (ret == NULL)
		{
			if (!skip_memory_init)
			{
				MessageBox(nullptr, L"初始化失败", OverlayTitle.data(),MB_OK);
				throw std::exception("初始化失败");
			}
		}

		return ret;
	}

	Memory::Pointers::Pattern Memory::Pointers::Pattern::rip() const
	{
		return add(GTA5.read <int>(address)).add(4);
	}

	Memory::Memory(std::wstring_view name): process_name(name)
	{
		PROCESSENTRY32 PE32;
		if (!ListSystemProcesses(const_cast <wchar_t*>(name.data()), &PE32))
		{
			MessageBox(nullptr, L"未找到 GTA5 进程", OverlayTitle.data(),MB_OK);
			throw std::exception("未找到 GTA5 进程");
		}
		process_id     = PE32.th32ProcessID;
		process_handle = OpenProcess(PROCESS_ALL_ACCESS, false, process_id);
		process_hwnd   = GetProcessMainWnd(process_id);
		if (!process_handle)
		{
			MessageBox(nullptr, L"未找到 GTA5 进程", OverlayTitle.data(),MB_OK);
			throw std::exception("未找到 GTA5 进程");
		}

		MODULEENTRY32 ME32;
		if (!ListProcessModules(process_id, name.data(), &ME32))
		{
			MessageBox(nullptr, L"未找到 GTA5 进程", OverlayTitle.data(),MB_OK);
			throw std::exception("未找到 GTA5 进程");
		}

		base_address = void_to_int64(ME32.modBaseAddr);
		process_size = ME32.modBaseSize;
	}

	std::string Memory::read_str(uintptr_t base, const size_t size, const std::vector <ptrdiff_t>& offsets) const
	{
		std::string str;
		str.reserve(size);
		for (const auto offset : offsets)
		{
			if (base == 0)
			{
				return {};
			}
			read_mem <uintptr_t>(base, &base);
			base = base + offset;
		}
		write_mem <char>(base, str.data(), size);
		return str;
	}

	void Memory::write_str(uintptr_t base, std::string_view str, const size_t size, const std::vector <ptrdiff_t>& offsets) const
	{
		for (const auto offset : offsets)
		{
			if (base == 0)
			{
				return;
			}
			read_mem <uintptr_t>(base, &base);
			base = base + offset;
		}
		read_mem <char>(base, const_cast <char* const>(str.data()), size);
	}
}
