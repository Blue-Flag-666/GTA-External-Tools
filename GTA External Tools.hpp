#pragma once

#include "pch.hpp"

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
