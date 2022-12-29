#include "pch.hpp"
#include "Memory.hpp"
#include "GTA External Tools.hpp"

HWND BF::GetProcessMainWnd(const DWORD dwProcessId)
{
	WNDINFO                   wndInfo { nullptr, dwProcessId };
	EnumWindows([](const HWND hWnd, const LPARAM lParam) ->BOOL
	{
		DWORD ProcessId = 0;
		GetWindowThreadProcessId(hWnd, &ProcessId);
		if (const auto pInfo = LPWNDINFO_t(lParam).info; ProcessId == pInfo->dwProcessId)
		{
			pInfo->hWnd = hWnd;
			return FALSE;
		}
		return TRUE;
	}, reinterpret_cast <LPARAM>(&wndInfo));

	return wndInfo.hWnd;
}

BOOL BF::ListSystemProcesses(WCHAR szExeFile[MAX_PATH], const LPPROCESSENTRY32 PE32)
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

BOOL BF::ListProcessModules(const UINT dwProcessId, const WCHAR szModule[MAX_MODULE_NAME32 + 1], const LPMODULEENTRY32 ME32)
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

UINT_PTR Memory::Pointers::Pattern::AOBScan(const string_view pattern)
{
	constexpr auto CHUNK_SIZE = 0x1000;

	vector <optional <UINT8>> compiled;

	UINT8 hexchar = 0;
	auto  first   = true, lastwaswc = false;

	for (const char c : pattern)
	{
		if (c == '?' && !lastwaswc)
		{
			lastwaswc = true;
			compiled.emplace_back(nullopt);
		}
		else
		{
			lastwaswc = false;
			if (c == ' ')
			{
				continue;
			}
			hexchar = hexchar + (first ? to_hex(c) * 0x10 : to_hex(c));

			if (!first)
			{
				compiled.emplace_back(hexchar);
				hexchar = 0;
			}
			first = !first;
		}
	}

	size_t     i      = GTA5.BaseAddr;
	const auto membuf = new UINT8[CHUNK_SIZE];

	UINT_PTR ret = 0;

	while (!ret)
	{
		if (i >= GTA5.BaseAddr + GTA5.Size)
		{
			break;
		}

		if (GTA5.read_memory <UINT8*>(LPVOID_t(i).ptr, membuf, CHUNK_SIZE))
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

	delete[] membuf;

	if (ret == NULL)
	{
		if (!skip_memory_init)
		{
			MessageBox(nullptr, L"初始化失败", L"GTA External Tools",MB_OK);
			throw exception("初始化失败");
		}
	}

	return ret;
}

Memory::Pointers::Pattern Memory::Pointers::Pattern::rip() const
{
	return add(GTA5.read <int>(address)).add(4);
}

Memory::Memory(wstring_view name): ProcessName(name)
{
	PROCESSENTRY32 PE32;
	if (!ListSystemProcesses(const_cast <WCHAR*>(name.data()), &PE32))
	{
		MessageBox(nullptr, L"未找到 GTA5 进程", L"GTA External Tools",MB_OK);
		throw exception("未找到 GTA5 进程");
	}
	ProcessID     = PE32.th32ProcessID;
	ProcessHandle = OpenProcess(PROCESS_ALL_ACCESS, false, ProcessID);
	ProcessHWND   = GetProcessMainWnd(ProcessID);
	if (!ProcessHandle)
	{
		MessageBox(nullptr, L"未找到 GTA5 进程", L"GTA External Tools",MB_OK);
		throw exception("未找到 GTA5 进程");
	}

	MODULEENTRY32 ME32;
	if (!ListProcessModules(ProcessID, name.data(), &ME32))
	{
		MessageBox(nullptr, L"未找到 GTA5 进程", L"GTA External Tools",MB_OK);
		throw exception("未找到 GTA5 进程");
	}

	BaseAddr = reinterpret_cast <UINT_PTR>(ME32.modBaseAddr);
	Size     = ME32.modBaseSize;
}

template <typename T> T Memory::read(DWORD_PTR BaseAddress, const vector <INT64>& offsets) const
{
	T ret = 0;
	for (const auto offset : offsets)
	{
		if (BaseAddress == 0)
		{
			return T();
		}
		read_memory <DWORD_PTR>(LPVOID_t(BaseAddress).ptr, &BaseAddress);
		BaseAddress = BaseAddress + offset;
	}
	read_memory <T>(LPVOID_t(BaseAddress).ptr, &ret);
	return ret;
}

template <typename T> void Memory::write(DWORD_PTR BaseAddress, T value, const vector <INT64>& offsets) const
{
	for (const auto offset : offsets)
	{
		if (BaseAddress == 0)
		{
			return;
		}
		read_memory <DWORD_PTR>(LPVOID_t(BaseAddress).ptr, &BaseAddress);
		BaseAddress = BaseAddress + offset;
	}
	write_memory <T>(LPVOID_t(BaseAddress).ptr, &value);
}

string Memory::read_str(DWORD_PTR BaseAddress, const SIZE_T nSize, const vector <INT64>& offsets) const
{
	const auto str = new char[nSize];
	memset(str, 0, nSize);
	for (const auto offset : offsets)
	{
		if (BaseAddress == 0)
		{
			delete[] str;
			return {};
		}
		read_memory <DWORD_PTR>(LPVOID_t(BaseAddress).ptr, &BaseAddress);
		BaseAddress = BaseAddress + offset;
	}
	write_memory <char>(LPVOID_t(BaseAddress).ptr, str, nSize);
	const auto ret = str;
	delete[] str;
	return ret;
}

void Memory::write_str(DWORD_PTR BaseAddress, string_view str, const SIZE_T nSize, const vector <INT64>& offsets) const
{
	for (const auto offset : offsets)
	{
		if (BaseAddress == 0)
		{
			return;
		}
		read_memory <DWORD_PTR>(LPVOID_t(BaseAddress).ptr, &BaseAddress);
		BaseAddress = BaseAddress + offset;
	}
	read_memory <char>(LPVOID_t(BaseAddress).ptr, const_cast <LPSTR>(str.data()), nSize);
}
