#pragma once

#include "pch.hpp"

namespace BF
{
	typedef struct ST_WNDINFO
	{
		HWND  hWnd;
		DWORD dwProcessId;
	}         WNDINFO, *LPWNDINFO;

	union LPWNDINFO_t
	{
		LPWNDINFO info;
		LPARAM    ptr;

		explicit LPWNDINFO_t(const LPARAM p): ptr(p) {}
	};

	union LPVOID_t
	{
		DWORD_PTR val;
		LPVOID    ptr;

		explicit LPVOID_t(const DWORD_PTR val): val(val) {}
	};

	HWND GetProcessMainWnd(DWORD dwProcessId);

	BOOL ListSystemProcesses(WCHAR szExeFile[MAX_PATH], LPPROCESSENTRY32 PE32);

	BOOL ListProcessModules(UINT dwProcessId, const WCHAR szModule[MAX_MODULE_NAME32 + 1], LPMODULEENTRY32 ME32);

	class Memory
	{
		wstring_view ProcessName;
		UINT         ProcessID     = NULL;
		HANDLE       ProcessHandle = nullptr;
		HWND         ProcessHWND   = nullptr;
		UINT_PTR     BaseAddr      = NULL;
		UINT         Size          = NULL;

		struct Pointers
		{
			UINT_PTR WorldPTR           = NULL;
			UINT_PTR BlipPTR            = NULL;
			UINT_PTR ReplayInterfacePTR = NULL;
			UINT_PTR LocalScriptsPTR    = NULL;
			UINT_PTR GlobalPTR          = NULL;
			UINT_PTR PlayerCountPTR     = NULL;
			UINT_PTR PickupDataPTR      = NULL;
			UINT_PTR WeatherADDR        = NULL;
			UINT_PTR SettingsPTRs       = NULL;
			UINT_PTR AimCPedPTR         = NULL;
			UINT_PTR FriendListPTR      = NULL;
			UINT_PTR ThermalADDR        = NULL;
			UINT_PTR NightVisionADDR    = NULL;
			UINT_PTR BlackoutADDR       = NULL;

			class Pattern
			{
				string_view name;
				UINT_PTR    address;

				static UINT_PTR AOBScan(string_view pattern);

				public:
					[[nodiscard]] Pattern rip() const;

					Pattern(const string_view name, const string_view pattern): name(name),
																				address(AOBScan(pattern)) {}

					Pattern(const string_view name, const UINT_PTR address): name(name),
																			 address(address) {}

					[[nodiscard]] Pattern add(const size_t n) const
					{
						return { name, address + n };
					}

					[[nodiscard]] Pattern sub(const size_t n) const
					{
						return { name, address - n };
					}

					[[nodiscard]] explicit operator UINT_PTR() const
					{
						return address;
					}
			};

			Pointers() = default;

			explicit Pointers(bool) : WorldPTR(Pattern("WorldPTR", "48 8B 05 ? ? ? ? 45 ? ? ? ? 48 8B 48 08 48 85 C9 74 07").add(3).rip()),
									  BlipPTR(Pattern("BlipPTR", "4C 8D 05 ? ? ? ? 0F B7 C1").add(3).rip()),
									  ReplayInterfacePTR(Pattern("ReplayInterfacePTR", "48 8D 0D ? ? ? ? 48 8B D7 E8 ? ? ? ? 48 8D 0D ? ? ? ? 8A D8 E8").add(3).rip()),
									  LocalScriptsPTR(Pattern("LocalScriptsPTR", "48 8B 05 ? ? ? ? 8B CF 48 8B 0C C8 39 59 68").add(3).rip()),
									  GlobalPTR(Pattern("GlobalPTR", "4C 8D 05 ? ? ? ? 4D 8B 08 4D 85 C9 74 11").add(3).rip()),
									  PlayerCountPTR(Pattern("PlayerCountPTR", "48 8B 0D ? ? ? ? E8 ? ? ? ? 48 8B C8 E8 ? ? ? ? 48 8B CF").add(3).rip()),
									  PickupDataPTR(Pattern("PickupDataPTR", "48 8B 05 ? ? ? ? 48 8B 1C F8 8B").add(3).rip()),
									  WeatherADDR(Pattern("WeatherADDR", "48 83 EC ? 8B 05 ? ? ? ? 8B 3D ? ? ? ? 49").add(6).rip().add(6)),
									  SettingsPTRs(Pattern("SettingsPTRs", "44 39 05 ? ? ? ? 75 0D").add(3).rip().add(0x89 - 4)),
									  AimCPedPTR(Pattern("AimCPedPTR", "48 8B 0D ? ? ? ? 48 85 C9 74 0C 48 8D 15 ? ? ? ? E8 ? ? ? ? 48 89 1D").add(3).rip()),
									  FriendListPTR(Pattern("FriendListPTR", "48 8B 0D ? ? ? ? 8B C6 48 8D 5C 24 70").add(3).rip()),
									  ThermalADDR(Pattern("ThermalADDR", "48 83 EC ? 80 3D ? ? ? ? 00 74 0C C6 81").add(6).rip().add(7)),
									  NightVisionADDR(Pattern("NightVisionADDR", "48 8B D7 48 8B C8 E8 ? ? ? ? 80 3D").add(13).rip().add(14)),
									  BlackoutADDR(Pattern("BlackoutADDR", "48 8B D1 8B 0D ? ? ? ? 45 8D 41 FC E9 ? ? ? ? 48 83").add(31).rip().add(31)) { }
		};

		public:
			Pointers pointers;

			Memory() = default;
			explicit Memory(wstring_view name);

			template <typename T> T    read(DWORD_PTR BaseAddress, const vector <INT64>& offsets = {}) const;
			template <typename T> void write(DWORD_PTR BaseAddress, T value, const vector <INT64>& offsets = {}) const;

			template <typename T> void write(DWORD_PTR BaseAddress, const vector <INT64>& offsets, T value) const
			{
				write(BaseAddress, value, offsets);
			}

			[[nodiscard]] string read_str(DWORD_PTR BaseAddress, SIZE_T nSize, const vector <INT64>& offsets = {}) const;
			void                 write_str(DWORD_PTR BaseAddress, string_view str, SIZE_T nSize, const vector <INT64>& offsets = {}) const;

			[[nodiscard]] bool is_empty() const
			{
				return ProcessID == NULL || ProcessHandle == nullptr || ProcessHWND == nullptr || BaseAddr == NULL || Size == NULL;
			}

			[[nodiscard]] bool is_running() const
			{
				return WaitForSingleObject(ProcessHandle, 0) == WAIT_TIMEOUT;
			}

			template <typename T> BOOL read_memory(const LPCVOID lpBaseAddress, const LPVOID lpBuffer, const SIZE_T nSize = sizeof(T), SIZE_T* lpNumberOfBytesRead = nullptr) const
			{
				return ReadProcessMemory(ProcessHandle, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesRead);
			}

			template <typename T> BOOL write_memory(const LPVOID lpBaseAddress, const LPCVOID lpBuffer, const SIZE_T nSize = sizeof(T), SIZE_T* lpNumberOfBytesWritten = nullptr) const
			{
				return WriteProcessMemory(ProcessHandle, lpBaseAddress, lpBuffer, nSize, lpNumberOfBytesWritten);
			}

			[[nodiscard]] INT64 get_global_addr(const int index) const
			{
				return read <INT64>(pointers.GlobalPTR + sizeof(INT64) * (index >> 0x12 & 0x3F)) + static_cast <INT64>(sizeof(INT64)) * (index & 0x3FFFF);
			}

			template <typename T> T get_global(const int index)
			{
				return read <T>(get_global_addr(index));
			}

			template <typename T> void set_global(const int index, T value)
			{
				write <T>(get_global_addr(index), value);
			}

			[[nodiscard]] INT64 get_local_addr(const string_view name, const int index) const
			{
				for (auto i = 0; i < 54; i++)
				{
					if (string str = read_str(pointers.LocalScriptsPTR, MAX_PATH, { static_cast <INT64>(i) * 0x8, 0xD0 }); str == name)
					{
						return read <INT64>(pointers.LocalScriptsPTR, { static_cast <INT64>(i) * 0x8, 0xB0 }) + 8ll * index;
					}
				}
				return NULL;
			}

			template <typename T> T get_local(const string_view name, const int index)
			{
				return read <T>(get_local_addr(name, index));
			}

			template <typename T> void set_local(const string_view name, const int index, T value)
			{
				write <T>(get_local_addr(name, index), {}, value);
			}

			[[nodiscard]] wstring_view name() const
			{
				return ProcessName;
			}

			[[nodiscard]] HANDLE handle() const
			{
				return ProcessHandle;
			}

			[[nodiscard]] static HWND find_window_hwnd()
			{
				return FindWindowA("grcWindow", nullptr);
			}

			[[nodiscard]] HWND hwnd() const
			{
				return ProcessHWND;
			}

			[[nodiscard]] UINT pid() const
			{
				return ProcessID;
			}

			[[nodiscard]] UINT_PTR baseAddr() const
			{
				return BaseAddr;
			}

			[[nodiscard]] UINT size() const
			{
				return Size;
			}
	};

	inline Memory GTA5;
}
