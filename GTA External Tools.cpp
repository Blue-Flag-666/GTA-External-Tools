#include "pch.hpp"
#include "GTA External Tools.hpp"
#include "Global Local Scanner.hpp"
#include "Memory.hpp"

using namespace BF;

// 全局变量:
ID3D11Device*           d3d_device                  = nullptr;
ID3D11DeviceContext*    d3d_device_context          = nullptr;
IDXGISwapChain*         d3d_swap_chain              = nullptr;
ID3D11RenderTargetView* d3d_main_render_target_view = nullptr;

HINSTANCE hInst;                    // 当前实例
HWND      OverlayHWND;

int APIENTRY wWinMain(_In_ const HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ const LPWSTR lpCmdLine, _In_ const int nShowCmd)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	ParseCmdLine(lpCmdLine);

	GTA5 = Memory(L"GTA5.exe");

	if (GTA5.empty() || !GTA5.running())
	{
		MessageBox(nullptr, L"未找到 GTA5 进程", OverlayTitle.data(),MB_OK);
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

	ImGui::GetIO().ConfigFlags = ImGui::GetIO().ConfigFlags | ImGuiConfigFlags_NavEnableKeyboard;

	ApplyStyle();

	static auto show_main_window = true, no_kill = true, done = false;

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

		if (done || !no_kill || !GTA5.running())
		{
			break;
		}

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		InitWindow(show_main_window, no_kill);

		ImGui::Render();

		constexpr float clear_color_with_alpha[4] = { 0, 0, 0, 0 };
		d3d_device_context->OMSetRenderTargets(1, &d3d_main_render_target_view, nullptr);
		d3d_device_context->ClearRenderTargetView(d3d_main_render_target_view, clear_color_with_alpha);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

		d3d_swap_chain->Present(1, NULL);
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
//  函数: WndProc(HWND, uint32_t, WPARAM, LPARAM)
//
//  目标: 处理主窗口的消息。
//
//  WM_COMMAND  - 处理应用程序菜单
//  WM_PAINT    - 绘制主窗口
//  WM_DESTROY  - 发送退出消息并返回
//
//
LRESULT CALLBACK WndProc(const HWND hWnd, const uint32_t msg, const WPARAM wParam, const LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
	{
		return TRUE;
	}
	switch (msg)
	{
		case WM_SIZE:
		{
			if (d3d_device != nullptr && wParam != SIZE_MINIMIZED)
			{
				CleanupRenderTarget();
				d3d_swap_chain->ResizeBuffers(0, LOWORD(lParam), HIWORD(lParam), DXGI_FORMAT_UNKNOWN, NULL);
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
	if (constexpr D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, }; D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, NULL, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &d3d_swap_chain, &d3d_device, &featureLevel, &d3d_device_context) != S_OK)
	{
		return false;
	}

	CreateRenderTarget();
	return true;
}

void CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (d3d_swap_chain)
	{
		d3d_swap_chain->Release();
		d3d_swap_chain = nullptr;
	}
	if (d3d_device_context)
	{
		d3d_device_context->Release();
		d3d_device_context = nullptr;
	}
	if (d3d_device)
	{
		d3d_device->Release();
		d3d_device = nullptr;
	}
}

void CreateRenderTarget()
{
	ID3D11Texture2D* pBackBuffer;
	d3d_swap_chain->GetBuffer(NULL, IID_PPV_ARGS(&pBackBuffer));
	if (!pBackBuffer)
	{
		MessageBox(nullptr, L"Get D3D11 Buffer Failed", L"Error",MB_OK);
		throw std::exception("Get D3D11 Buffer Failed");
	}
	d3d_device->CreateRenderTargetView(pBackBuffer, nullptr, &d3d_main_render_target_view);
	pBackBuffer->Release();
}

void CleanupRenderTarget()
{
	if (d3d_main_render_target_view)
	{
		d3d_main_render_target_view->Release();
		d3d_main_render_target_view = nullptr;
	}
}

void ParseCmdLine(const LPWSTR lpCmdLine)
{
	auto                                  cnt       = 0;
	const LPWSTR*                         szArgList = CommandLineToArgvW(lpCmdLine, &cnt);
	const std::vector <std::wstring_view> args(szArgList, szArgList + cnt);

	for (const auto& [arg_str, func] : std::initializer_list <std::pair <std::wstring_view, std::function <void()>>> {
			 std::make_pair(L"--skip-memory-init", []
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
	const auto dpi = static_cast <float>(GetDpiForWindow(OverlayHWND)) * 96.0f / 100.0f / 100.f;

	ImGui::StyleColorsDark();
	// ImGui::StyleColorsLight();

	ImGui::GetStyle().ScaleAllSizes(dpi);

	ImGui_ImplWin32_Init(OverlayHWND);
	ImGui_ImplDX11_Init(d3d_device, d3d_device_context);

	if (constexpr std::string_view filename = R"(C:\Windows\Fonts\segoeui.ttf)"; std::filesystem::exists(filename))
	{
		const ImFontConfig config;
		ImGui::GetIO().Fonts->AddFontFromFileTTF(filename.data(), round(18.0f * dpi), &config, ImGui::GetIO().Fonts->GetGlyphRangesChineseSimplifiedCommon());
	}
	else
	{
		ImGui::GetIO().Fonts->AddFontDefault();
	}
}

void InitWindow(bool& show_main_window, bool& no_kill)
{
	if (ImGui::IsKeyPressed(ImGuiKey_Insert, false))
	{
		show_main_window = !show_main_window;
	}
	if (show_main_window)
	{
		WINDOWINFO info = {};
		GetWindowInfo(Memory::find(), &info);
		MoveWindow(OverlayHWND, info.rcClient.left, info.rcClient.top, info.rcClient.right - info.rcClient.left, info.rcClient.bottom - info.rcClient.top, TRUE);
		MainWindow(no_kill);
	}
}

void MainWindow(bool& no_kill)
{
	ImGui::ShowDemoWindow();//debug

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
			static auto first_run_global_local_scanner = true;
			Global_Local_Scanner::show(show_global_local_scanner, first_run_global_local_scanner);
		}

		ImGui::Text("Date: %s", std::format("{0:%Y-%m-%d}", std::chrono::zoned_time(std::chrono::current_zone(), std::chrono::system_clock::now())).c_str());
		ImGui::Text("Time: %s", std::format("{0:%I:%M:%S %p}", std::chrono::zoned_time(std::chrono::current_zone(), std::chrono::round <std::chrono::seconds>(std::chrono::system_clock::now()))).c_str());
	}
	ImGui::End();
}
