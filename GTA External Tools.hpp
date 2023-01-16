#pragma once

#include "pch.hpp"

// 全局变量:
static ID3D11Device*           d3d_device                  = nullptr;
static ID3D11DeviceContext*    d3d_device_context          = nullptr;
static IDXGISwapChain*         d3d_swap_chain              = nullptr;
static ID3D11RenderTargetView* d3d_main_render_target_view = nullptr;

inline bool skip_memory_init = false;

// 此代码模块中包含的函数的前向声明:
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, uint32_t msg, WPARAM wParam, LPARAM lParam);

ATOM             MyRegisterClass(HINSTANCE hInstance);
BOOL             InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, uint32_t, WPARAM, LPARAM);

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();

void ParseCmdLine(LPWSTR lpCmdLine);

void ApplyStyle();
void InitWindow(bool& show_main_window, bool& no_kill);
void MainWindow(bool& no_kill);
void GlobalLocalScanner(bool& show, bool& first);

void                                         ScanGlobalLocal(std::set <BF::scan_result>& s, int scan_type, const BF::scan_value& value);
template <typename T, BF::scan_type ST> void ScanGlobalLocal(std::set <BF::scan_result>& s, T value);
template <> void                             ScanGlobalLocal <char*, BF::SCAN_STRING>(std::set <BF::scan_result>& s, char* value);
