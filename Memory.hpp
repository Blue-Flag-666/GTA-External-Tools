#pragma once

#include "pch.hpp"

using WNDINFO = struct ST_WNDINFO
{
	HWND  hWnd;
	DWORD dwProcessId;
};
using LPWNDINFO = WNDINFO*;

HWND GetProcessMainWnd(DWORD dwProcessId);

BOOL ListSystemProcesses(WCHAR szExeFile[MAX_PATH], LPPROCESSENTRY32 PE32);

BOOL ListProcessModules(uint32_t dwProcessId, const WCHAR szModule[MAX_MODULE_NAME32 + 1], LPMODULEENTRY32 ME32);

namespace BF
{
	class Memory
	{
		std::wstring_view process_name;
		uint32_t          process_id     = NULL;
		HANDLE            process_handle = nullptr;
		HWND              process_hwnd   = nullptr;
		uintptr_t         base_address   = NULL;
		uint32_t          process_size   = NULL;

		struct Pointers
		{
			uintptr_t WorldPTR           = NULL;
			uintptr_t BlipPTR            = NULL;
			uintptr_t ReplayInterfacePTR = NULL;
			uintptr_t LocalScriptsPTR    = NULL;
			uintptr_t GlobalPTR          = NULL;
			uintptr_t PlayerCountPTR     = NULL;
			uintptr_t PickupDataPTR      = NULL;
			uintptr_t WeatherADDR        = NULL;
			uintptr_t SettingsPTRs       = NULL;
			uintptr_t AimCPedPTR         = NULL;
			uintptr_t FriendListPTR      = NULL;
			uintptr_t ThermalADDR        = NULL;
			uintptr_t NightVisionADDR    = NULL;
			uintptr_t BlackoutADDR       = NULL;

			class Pattern
			{
				std::string_view name;
				uintptr_t        address;

				static uintptr_t AOBScan(std::string_view pattern);

				public:
					[[nodiscard]] Pattern rip() const;

					Pattern(const std::string_view name, const std::string_view pattern): name(name),
																						  address(AOBScan(pattern)) {}

					Pattern(const std::string_view name, const uintptr_t address): name(name),
																				   address(address) {}

					[[nodiscard]] Pattern add(const size_t n) const
					{
						return { name, address + n };
					}

					[[nodiscard]] Pattern sub(const size_t n) const
					{
						return { name, address - n };
					}

					[[nodiscard]] explicit operator uintptr_t() const
					{
						return address;
					}
			};

			Pointers() = default;

			explicit Pointers(bool /*unused*/) : WorldPTR(Pattern("WorldPTR", "48 8B 05 ? ? ? ? 45 ? ? ? ? 48 8B 48 08 48 85 C9 74 07").add(3).rip()),
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
												 BlackoutADDR(Pattern("BlackoutADDR", "48 8B D1 8B 0D ? ? ? ? 45 8D 41 FC E9 ? ? ? ? 48 83").add(31).rip().add(31)) {}
		};

		public:
			Pointers pointers;

			Memory() = default;
			explicit Memory(std::wstring_view name);

			template <typename T> [[nodiscard]] T read(uintptr_t BaseAddress, const std::vector <int64_t>& offsets = {}) const
			{
				T ret = 0;
				for (const auto offset : offsets)
				{
					if (BaseAddress == 0)
					{
						return T();
					}
					read_memory <uintptr_t>(BaseAddress, &BaseAddress);
					BaseAddress = BaseAddress + offset;
				}
				read_memory <T>(BaseAddress, &ret);
				return ret;
			}

			template <typename T> void write(uintptr_t BaseAddress, T value, const std::vector <int64_t>& offsets = {}) const
			{
				for (const auto offset : offsets)
				{
					if (BaseAddress == 0)
					{
						return;
					}
					read_memory <uintptr_t>(BaseAddress, &BaseAddress);
					BaseAddress = BaseAddress + offset;
				}
				write_memory <T>(BaseAddress, &value);
			}

			template <typename T> void write(uintptr_t BaseAddress, const std::vector <int64_t>& offsets, T value) const
			{
				write(BaseAddress, value, offsets);
			}

			[[nodiscard]] std::string read_str(uintptr_t BaseAddress, size_t nSize, const std::vector <int64_t>& offsets = {}) const;
			void                      write_str(uintptr_t BaseAddress, std::string_view str, size_t nSize, const std::vector <int64_t>& offsets = {}) const;

			[[nodiscard]] bool is_empty() const
			{
				return process_id == NULL || process_handle == nullptr || process_hwnd == nullptr || base_address == NULL || process_size == NULL;
			}

			[[nodiscard]] bool is_running() const
			{
				return WaitForSingleObject(process_handle, 0) == WAIT_TIMEOUT;
			}

			template <typename T> bool read_memory(const uintptr_t lpBaseAddress, void* const lpBuffer, const size_t nSize = sizeof(T), size_t* const lpNumberOfBytesRead = nullptr) const
			{
				return ReadProcessMemory(process_handle, reinterpret_cast <void*>(lpBaseAddress), lpBuffer, nSize, lpNumberOfBytesRead);
			}

			template <typename T> bool write_memory(const uintptr_t lpBaseAddress, const void* const lpBuffer, const size_t nSize = sizeof(T), size_t* const lpNumberOfBytesWritten = nullptr) const
			{
				return WriteProcessMemory(process_handle, reinterpret_cast <void*>(lpBaseAddress), lpBuffer, nSize, lpNumberOfBytesWritten);
			}

			[[nodiscard]] int64_t get_global_addr(const int index) const
			{
				return read <int64_t>(pointers.GlobalPTR + sizeof(int64_t) * (index >> 0x12 & 0x3F)) + static_cast <int64_t>(sizeof(int64_t)) * (index & 0x3FFFF);
			}

			template <typename T> [[nodiscard]] T get_global(const int index)
			{
				return read <T>(get_global_addr(index));
			}

			template <typename T> void set_global(const int index, T value)
			{
				write <T>(get_global_addr(index), value);
			}

			[[nodiscard]] int64_t get_local_addr(const std::string_view name, const int index) const
			{
				for (auto i = 0; i < 54; i++)
				{
					if (const std::string str = read_str(pointers.LocalScriptsPTR, MAX_PATH, { static_cast <int64_t>(i) * 0x8, 0xD0 }); str == name)
					{
						return read <int64_t>(pointers.LocalScriptsPTR, { static_cast <int64_t>(i) * 0x8, 0xB0 }) + 8ll * index;
					}
				}
				return NULL;
			}

			template <typename T> [[nodiscard]] T get_local(const std::string_view name, const int index)
			{
				return read <T>(get_local_addr(name, index));
			}

			template <typename T> void set_local(const std::string_view name, const int index, T value)
			{
				write <T>(get_local_addr(name, index), {}, value);
			}

			[[nodiscard]] std::wstring_view name() const
			{
				return process_name;
			}

			[[nodiscard]] HANDLE handle() const
			{
				return process_handle;
			}

			[[nodiscard]] static HWND find_window_hwnd()
			{
				return FindWindow(L"grcWindow", nullptr);
			}

			[[nodiscard]] HWND hwnd() const
			{
				return process_hwnd;
			}

			[[nodiscard]] uint32_t pid() const
			{
				return process_id;
			}

			[[nodiscard]] uintptr_t baseAddr() const
			{
				return base_address;
			}

			[[nodiscard]] uint32_t size() const
			{
				return process_size;
			}
	};

	inline Memory GTA5;
}
