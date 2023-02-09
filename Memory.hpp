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
			uintptr_t global_ptr        = NULL;
			uintptr_t local_scripts_ptr = NULL;

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

			explicit Pointers(bool) : global_ptr(Pattern("GlobalPTR", "4C 8D 05 ? ? ? ? 4D 8B 08 4D 85 C9 74 11").add(3).rip()),
									  local_scripts_ptr(Pattern("LocalScriptsPTR", "48 8B 05 ? ? ? ? 8B CF 48 8B 0C C8 39 59 68").add(3).rip()) {}
		};

		public:
			Pointers pointers;

			Memory() = default;
			explicit Memory(std::wstring_view name);

			template <typename T> [[nodiscard]] T read(uintptr_t base, const std::vector <ptrdiff_t>& offsets = {}) const
			{
				T ret = 0;
				for (const auto offset : offsets)
				{
					if (base == 0)
					{
						return T();
					}
					read_mem <uintptr_t>(base, &base);
					base = base + offset;
				}
				read_mem <T>(base, &ret);
				return ret;
			}

			template <typename T> void write(uintptr_t base, T value, const std::vector <ptrdiff_t>& offsets = {}) const
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
				write_mem <T>(base, &value);
			}

			template <typename T> void write(uintptr_t base, const std::vector <ptrdiff_t>& offsets, T value) const
			{
				write(base, value, offsets);
			}

			[[nodiscard]] std::string read_str(uintptr_t base, size_t size, const std::vector <ptrdiff_t>& offsets = {}) const;
			void                      write_str(uintptr_t base, std::string_view str, size_t size, const std::vector <ptrdiff_t>& offsets = {}) const;

			[[nodiscard]] bool empty() const
			{
				return process_id == NULL || process_handle == nullptr || process_hwnd == nullptr || base_address == NULL || process_size == NULL;
			}

			[[nodiscard]] bool running() const
			{
				return WaitForSingleObject(process_handle, 0) == WAIT_TIMEOUT;
			}

			template <typename T> bool read_mem(const uintptr_t base, void* const buffer, const size_t size = sizeof(T), size_t* const number_of_bytes_read = nullptr) const
			{
				return ReadProcessMemory(process_handle, int64_to_void(base), buffer, size, number_of_bytes_read);
			}

			template <typename T> bool write_mem(const uintptr_t base, const void* const buffer, const size_t size = sizeof(T), size_t* const number_of_bytes_written = nullptr) const
			{
				return WriteProcessMemory(process_handle, int64_to_void(base), buffer, size, number_of_bytes_written);
			}

			[[nodiscard]] int64_t get_global_addr(const int index) const
			{
				return read <ptrdiff_t>(pointers.global_ptr + sizeof(ptrdiff_t) * (index >> 0x12 & 0x3F)) + static_cast <ptrdiff_t>(sizeof(ptrdiff_t)) * (index & 0x3FFFF);
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
					if (const std::string str = read_str(pointers.local_scripts_ptr, MAX_PATH, { static_cast <ptrdiff_t>(i) * 0x8, 0xD0 }); str == name)
					{
						return read <ptrdiff_t>(pointers.local_scripts_ptr, { static_cast <ptrdiff_t>(i) * 0x8, 0xB0 }) + 8ll * index;
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

			[[nodiscard]] static HWND find()
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

			[[nodiscard]] uintptr_t base_addr() const
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
