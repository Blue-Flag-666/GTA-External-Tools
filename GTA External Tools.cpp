// GTA External Tools.cpp

#include "pch.hpp"
#include "GTA External Tools.hpp"

#include "Memory.hpp"

// 全局变量:
HINSTANCE hInst;                    // 当前实例
HWND      OverlayHWND;

auto show_main_window = true, no_kill = true, done = false;

int APIENTRY wWinMain(_In_ const HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ const LPWSTR lpCmdLine, _In_ const int nShowCmd)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	ParseCmdLine(lpCmdLine);

	GTA5 = Memory(L"GTA5.exe");

	if (GTA5.is_empty() || !GTA5.is_running())
	{
		MessageBox(nullptr, L"未找到 GTA5 进程", L"GTA External Tools",MB_OK);
		return 0;
	}

	MyRegisterClass(hInstance);

	// 执行应用程序初始化:
	if (!InitInstance(hInstance, nShowCmd))
	{
		return FALSE;
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ApplyStyle();

	while (!done)
	{
		MSG msg;
		while (PeekMessage(&msg, nullptr, NULL, NULL, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
			{
				done = true;
			}
		}

		if (done || !no_kill || !GTA5.is_running())
		{
			break;
		}

		if (GetAsyncKeyState(VK_INSERT) & 0x1)
		{
			show_main_window = !show_main_window;
		}

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		InitWindow();

		ImGui::Render();

		constexpr auto  clear_color               = ImVec4(0, 0, 0, 0);
		constexpr float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		g_pSwapChain->Present(1, NULL);
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	DestroyWindow(OverlayHWND);
	UnregisterClassW(OverlayTitle.data(), hInstance);

	return 0;
}

//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(const HINSTANCE hInstance)
{
	const WNDCLASSEXW wcex { sizeof(WNDCLASSEXW),CS_HREDRAW | CS_VREDRAW, WndProc, NULL, NULL, hInstance,LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GTAEXTERNALTOOLS)),LoadCursor(nullptr,IDC_ARROW), CreateSolidBrush(RGB(0, 0, 0)), OverlayTitle.data(), OverlayTitle.data(), nullptr };

	return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目标: 保存实例句柄并创建主窗口
//
//   注释:
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(const HINSTANCE hInstance, const int nCmdShow)
{
	hInst = hInstance; // 将实例句柄存储在全局变量中

	OverlayHWND = CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW, OverlayTitle.data(), OverlayTitle.data(), WS_POPUP, 0, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!CreateDeviceD3D(OverlayHWND))
	{
		CleanupDeviceD3D();
		UnregisterClassW(OverlayTitle.data(), hInstance);
		return FALSE;
	}

	SetLayeredWindowAttributes(OverlayHWND, RGB(0, 0, 0), 0, LWA_COLORKEY);

	ShowWindow(OverlayHWND, nCmdShow);
	UpdateWindow(OverlayHWND);

	return TRUE;
}

//
//  函数: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(const HWND hWnd, const UINT msg, const WPARAM wParam, const LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
	{
		return true;
	}
	switch (msg)
	{
		case WM_SIZE:
		{
			if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
			{
				CleanupRenderTarget();
				g_pSwapChain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, NULL);
				CreateRenderTarget();
			}
			return 0;
		}
		case WM_SYSCOMMAND:
		{
			if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			{
				return 0;
			}
			break;
		}
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
		default:
		{
			return DefWindowProc(hWnd, msg, wParam, lParam);
		}
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}

bool CreateDeviceD3D(const HWND hWnd)
{
	const DXGI_SWAP_CHAIN_DESC sd = { { 0, 0, { 60, 1 }, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED, DXGI_MODE_SCALING_UNSPECIFIED }, { 1, 0 },DXGI_USAGE_RENDER_TARGET_OUTPUT, 2, hWnd,TRUE, DXGI_SWAP_EFFECT_DISCARD, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH };

	D3D_FEATURE_LEVEL featureLevel;
	if (constexpr D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, }; D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, NULL, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
	{
		return false;
	}

	CreateRenderTarget();
	return true;
}

void CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (g_pSwapChain)
	{
		g_pSwapChain->Release();
		g_pSwapChain = nullptr;
	}
	if (g_pd3dDeviceContext)
	{
		g_pd3dDeviceContext->Release();
		g_pd3dDeviceContext = nullptr;
	}
	if (g_pd3dDevice)
	{
		g_pd3dDevice->Release();
		g_pd3dDevice = nullptr;
	}
}

void CreateRenderTarget()
{
	ID3D11Texture2D* pBackBuffer;
	g_pSwapChain->GetBuffer(NULL, IID_PPV_ARGS(&pBackBuffer));
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
	pBackBuffer->Release();
}

void CleanupRenderTarget()
{
	if (g_mainRenderTargetView)
	{
		g_mainRenderTargetView->Release();
		g_mainRenderTargetView = nullptr;
	}
}

void ParseCmdLine(const LPWSTR lpCmdLine)
{
	auto                        cnt       = 0;
	const LPWSTR*               szArgList = CommandLineToArgvW(lpCmdLine, &cnt);
	const vector <wstring_view> args(szArgList, szArgList + cnt);

	for (const auto& [arg_str, func] : initializer_list <pair <wstring_view, function <void()>>> {
			 make_pair(L"--skip-memory-init", []
			 {
				 MessageBox(nullptr, L"Memory Init Skipped", OverlayTitle.data(), MB_OK);
				 skip_memory_init = true;
			 })
		 })
	{
		for (auto& arg : args)
		{
			if (arg_str == arg)
			{
				func();
			}
		}
	}
}

void ApplyStyle()
{
	const auto     dpi = static_cast <float>(GetDpiForWindow(OverlayHWND)) * 96.0f / 100.0f / 100.f;
	const ImGuiIO& io  = ImGui::GetIO();

	ImGui::StyleColorsDark();
	// ImGui::StyleColorsLight();

	ImGui::GetStyle().ScaleAllSizes(dpi);

	ImGui_ImplWin32_Init(OverlayHWND);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	if (constexpr string_view filename = R"(C:\Windows\Fonts\Segoeui.ttf)"; filesystem::exists(filename))
	{
		io.Fonts->AddFontFromFileTTF(filename.data(), round(18.0f * dpi));
	}
	else
	{
		io.Fonts->AddFontDefault();
	}
}

void InitWindow()
{
	if (show_main_window)
	{
		WINDOWINFO info;
		GetWindowInfo(FindWindowA("grcWindow", nullptr), &info);
		MoveWindow(OverlayHWND, info.rcClient.left, info.rcClient.top, info.rcClient.right - info.rcClient.left, info.rcClient.bottom - info.rcClient.top, true);
		MainWindow();
	}
}

void MainWindow()
{
	ImGui::Begin("GTA External Tools", &no_kill, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
	{
		static auto show_global_local_scanner = false;

		ImGui::Text("Hello!");

		if (ImGui::Button("Global/Local Scanner"))
		{
			show_global_local_scanner = !show_global_local_scanner;
		}

		if (show_global_local_scanner)
		{
			GlobalLocalScanner(show_global_local_scanner);
		}

		{
			const time_t cur_time  = time(nullptr);
			tm           time_info = {};
			if (!localtime_s(&time_info, &cur_time))
			{
				constexpr auto MAX_STR_LEN = 0xff;
				char           str[MAX_STR_LEN];
				(void) strftime(str, MAX_STR_LEN, "%Y-%m-%d %H:%M:%S", &time_info);
				ImGui::Text("%s", str);
			}
		}
	}
	ImGui::End();
}

void GlobalLocalScanner(bool& show)
{
	ImGui::Begin("Global/Local Scanner", &show, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);
	{
		ImGui::Text("Hello!");
		//TODO
	}
	ImGui::End();
}
