#pragma once

#include "pch.hpp"

// 全局变量:
static ID3D11Device*           g_pd3dDevice           = nullptr;
static ID3D11DeviceContext*    g_pd3dDeviceContext    = nullptr;
static IDXGISwapChain*         g_pSwapChain           = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

inline bool skip_memory_init = false;

// 此代码模块中包含的函数的前向声明:
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

ATOM             MyRegisterClass(HINSTANCE hInstance);
BOOL             InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();

void ParseCmdLine(LPWSTR lpCmdLine);

void ApplyStyle();
void InitWindow();
void MainWindow();
void GlobalLocalScanner(bool& show, bool& first);

enum Scan_Type
{
	SCAN_BOOL   = 0b1,
	SCAN_FLOAT  = 0b10,
	SCAN_INT    = 0b100,
	SCAN_UINT   = 0b1000,
	SCAN_STRING = 0b10000,
	SCAN_NUMBER = 0b1110,
	SCAN_NONE   = 0
};

string to_string(Scan_Type type);

struct Scan_Value
{
	bool   bool_value  = true;
	float  float_value = 0;
	int    int_value   = 0;
	UINT   uint_value  = 0;
	string str_value;

	Scan_Value() = default;

	explicit Scan_Value(const bool value): bool_value(value) {}

	explicit Scan_Value(const double value): float_value(static_cast <float>(value)),
											 int_value(static_cast <int>(value)),
											 uint_value(static_cast <UINT>(value)) {}

	explicit Scan_Value(const int value): int_value(value),
										  uint_value(value) {}

	explicit Scan_Value(const UINT value): uint_value(value) {}
	explicit Scan_Value(const string_view value): str_value(value) {}
};

struct Scan_Result
{
	int       Offset = 0;
	Scan_Type Type   = SCAN_INT;

	Scan_Result() = delete;

	Scan_Result(const int offset, const Scan_Type type) : Offset(offset),
														  Type(type) {}

	bool operator<(const Scan_Result& x) const
	{
		return Offset < x.Offset || (Offset == x.Offset && Type < x.Type);
	}
};

struct Scan_Item : Scan_Result
{
	string Name = "Unnamed";

	Scan_Item(const int offset, const Scan_Type type): Scan_Result(offset, type) {}
};

void                                     ScanGlobalLocal(set <Scan_Result>& s, int scan_type, const Scan_Value& value);
template <typename T, Scan_Type ST> void ScanGlobalLocal(set <Scan_Result>& s, T value);
template <> void                         ScanGlobalLocal <string_view, SCAN_STRING>(set <Scan_Result>& s, string_view value);
