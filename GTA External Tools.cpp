#include "pch.hpp"
#include "GTA External Tools.hpp"
#include "Memory.hpp"

using namespace BF;

// 全局变量:
HINSTANCE hInst;                    // 当前实例
HWND      OverlayHWND;

int APIENTRY wWinMain(_In_ const HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ const LPWSTR lpCmdLine, _In_ const int nShowCmd)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	ParseCmdLine(lpCmdLine);

	GTA5 = Memory(L"GTA5.exe");

	if (GTA5.is_empty() || !GTA5.is_running())
	{
		MessageBox(nullptr, L"未找到 GTA5 进程", overlay_title.data(),MB_OK);
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

		if (done || !no_kill || !GTA5.is_running())
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
	UnregisterClassW(overlay_title.data(), hInstance);

	return 0;
}

//
//  函数: MyRegisterClass()
//
//  目标: 注册窗口类。
//
ATOM MyRegisterClass(const HINSTANCE hInstance)
{
	const WNDCLASSEXW wcex { sizeof(WNDCLASSEXW),CS_HREDRAW | CS_VREDRAW, WndProc, NULL, NULL, hInstance,LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GTAEXTERNALTOOLS)),LoadCursor(nullptr,IDC_ARROW), CreateSolidBrush(RGB(0, 0, 0)), overlay_title.data(), overlay_title.data(), nullptr };

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

	OverlayHWND = CreateWindowExW(WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW, overlay_title.data(), overlay_title.data(), WS_POPUP, 0, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

	if (!CreateDeviceD3D(OverlayHWND))
	{
		CleanupDeviceD3D();
		UnregisterClassW(overlay_title.data(), hInstance);
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
				 MessageBox(nullptr, L"Memory Init Skipped", overlay_title.data(), MB_OK);
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
		GetWindowInfo(Memory::find_window_hwnd(), &info);
		MoveWindow(OverlayHWND, info.rcClient.left, info.rcClient.top, info.rcClient.right - info.rcClient.left, info.rcClient.bottom - info.rcClient.top, TRUE);
		MainWindow(no_kill);
	}
}

void MainWindow(bool& no_kill)
{
	ImGui::ShowDemoWindow();

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
			GlobalLocalScanner(show_global_local_scanner, first_run_global_local_scanner);
		}

		{
			const time_t cur_time  = time(nullptr);
			tm           time_info = {};
			if (!localtime_s(&time_info, &cur_time))
			{
				char str[MAX_STR_LEN];
				(void) strftime(str, MAX_STR_LEN, "%Y-%m-%d %H:%M:%S", &time_info);
				ImGui::Text("%s", str);
			}
		}
	}
	ImGui::End();
}

void GlobalLocalScanner(bool& show, bool& first)
{
	ImGui::Begin("Global/Local Scanner", &show, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);
	{
		if (first)
		{
			ImGui::SetWindowSize(ImVec2(570, 0));
			first = false;
		}
		static std::vector <scan_item> list_addr;
		static auto                    begin          = 1,   end         = 5000000;
		static float                   scanner_height = 220, list_height = 200;

		if (static float last_height = 420; !equal(last_height, ImGui::GetContentRegionAvail().y))
		{
			const float height = ImGui::GetContentRegionAvail().y - ImGui::GetStyle().ItemSpacing.y;
			scanner_height     = max(220, height/(last_height-ImGui::GetStyle().ItemSpacing.y)*scanner_height);
			list_height        = max(200, height - scanner_height);
			scanner_height     = max(220, height - list_height);
			last_height        = ImGui::GetContentRegionAvail().y;
		}

		ImGui::Splitter(false, 3, &scanner_height, &list_height, 220, 200);

		ImGui::BeginChild("Scanner", ImVec2(0, scanner_height), false, ImGuiWindowFlags_AlwaysAutoResize);
		{
			static std::set <scan_result>        scan_addr;
			static std::set <const scan_result*> selection;
			static float                         option_width = 270, result_width = 270;

			if (static float last_width = 540; !equal(last_width, ImGui::GetContentRegionAvail().x))
			{
				const float width = ImGui::GetContentRegionAvail().x - ImGui::GetStyle().ItemSpacing.x;
				option_width      = max(270, width/(last_width - ImGui::GetStyle().ItemSpacing.x)*option_width);
				result_width      = max(270, width-option_width);
				option_width      = max(270, width-result_width);
				last_width        = ImGui::GetContentRegionAvail().x;
			}

			ImGui::Splitter(true, 3, &option_width, &result_width, 270, 270);

			ImGui::BeginChild("Scan Option", ImVec2(option_width, 0));
			{
				static scan_value value;
				static int        scan_type = SCAN_INT;
				if (ImGui::Button("First Scan"))
				{
					scan_addr.clear();
					ScanGlobalLocal(scan_addr, scan_type, value);
				}
				ImGui::SameLine();
				if (ImGui::Button("Next Scan"))
				{
					ScanGlobalLocal(scan_addr, scan_type, value);
				}
				ImGui::SameLine();
				ImGui::Spacing(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(("Count: " + std::to_string(scan_addr.size())).c_str()).x - ImGui::GetStyle().ItemSpacing.x - 3 - 1, 0);
				ImGui::SameLine();
				ImGui::Text("Count: %lld", scan_addr.size());

				{
					const char* items[]  = { "Bool", "Number", "String" };
					static auto cur_item = 1;
					ImGui::Combo("Scan", &cur_item, items,IM_ARRAYSIZE(items));

					switch (cur_item)
					{
						case 0:
						{
							scan_type = SCAN_BOOL;
							break;
						}
						case 1:
						{
							if (scan_type & SCAN_NUMBER)
							{
								bool scan_float = scan_type & SCAN_FLOAT, scan_int = scan_type & SCAN_INT, scan_uint = scan_type & SCAN_UINT;
								ImGui::Checkbox("Float", &scan_float);
								ImGui::SameLine();
								ImGui::Checkbox("Int", &scan_int);
								ImGui::SameLine();
								ImGui::Checkbox("UInt", &scan_uint);
								if (const auto new_scan_type = (scan_float ? SCAN_FLOAT : SCAN_NONE) | (scan_int ? SCAN_INT : SCAN_NONE) | (scan_uint ? SCAN_UINT : SCAN_NONE))
								{
									scan_type = new_scan_type;
								}
							}
							else
							{
								scan_type = SCAN_INT;
							}
							break;
						}
						case 2:
						{
							scan_type = SCAN_STRING;
							break;
						}
						default: ;
					}
				}

				if (scan_type & SCAN_BOOL)
				{
					value          = scan_value(value.bool_value);
					int bool_value = value.bool_value;
					ImGui::RadioButton("True", &bool_value, true);
					ImGui::SameLine();
					ImGui::RadioButton("False", &bool_value, false);
					value.bool_value = bool_value;
				}
				else if (scan_type & SCAN_STRING)
				{
					value.reset(value.string_value);
					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("Value").x - ImGui::CalcTextSize(("Length: " + std::to_string(strlen(value.string_value))).c_str()).x - ImGui::GetStyle().ItemSpacing.x * 2 - 3);
					ImGui::InputText("Value", value.string_value, MAX_STR_LEN);
					ImGui::SameLine();
					ImGui::Text("Length: %lld", strlen(value.string_value));
					if (strlen(value.string_value) == static_cast <size_t>(MAX_STR_LEN - 1))
					{
						ImGui::Text("Warning: Max string length reached. (%d)", MAX_STR_LEN - 1);
					}
				}
				else
				{
					if (scan_type & SCAN_FLOAT)
					{
						auto x = static_cast <double>(value.float_value);
						ImGui::InputDouble("Value", &x);
						value = scan_value(x);
					}
					else if (scan_type & SCAN_UINT)
					{
						uint32_t x = value.uint_value;
						ImGui::InputScalar("Value", ImGuiDataType_U32, &x);
						value = scan_value(x);
					}
					else
					{
						int x = value.int_value;
						ImGui::InputInt("Value", &x, 0);
						value = scan_value(x);
					}
				}

				ImGui::Separator();
				ImGui::Text("Scan Options");
				ImGui::InputInt("Begin", &begin, 0);
				ImGui::InputInt("End", &end, 0);

				ImGui::SetCursorScreenPos(ImGui::GetContentRegionMaxAbs() - ImVec2(ImGui::CalcTextSize("Add").x + ImGui::GetStyle().ItemSpacing.x + 3, ImGui::GetTextLineHeight() + ImGui::GetStyle().ItemSpacing.y * 2 + 3));
				if (ImGui::Button("Add"))
				{
					for (const auto x : selection)
					{
						list_addr.emplace_back(x->offset, x->type);
					}
				}
			}
			ImGui::EndChild();

			ImGui::SameLine();
			ImGui::BeginChild("Scan Result", ImVec2(result_width, ImGui::GetContentRegionAvail().y - ImGui::GetStyle().ItemSpacing.y));
			{
				/*if (scan_addr.empty())//debug
				{
					for (auto i = 1; i <= 100; i++)
					{
						scan_addr.insert({ i * 8, SCAN_BOOL });
					}
				}*/

				if (ImGui::BeginTable("Result Table", 4, (result_width < 320 ? ImGuiTableFlags_SizingFixedFit : ImGuiTableFlags_None) | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersH | ImGuiTableFlags_ScrollY))
				{
					ImGui::TableSetupColumn("Offset");
					ImGui::TableSetupColumn("Address");
					ImGui::TableSetupColumn("Value");
					ImGui::TableSetupColumn("Previous");
					ImGui::TableSetupScrollFreeze(0, 1);
					ImGui::TableHeadersRow();
					{
						for (auto item = scan_addr.begin(); item != scan_addr.end(); ++item)
						{
							const bool selected = selection.contains(&*item);

							ImGui::TableNextRow();

							if (ImGui::TableSetColumnIndex(0))
							{
								if (ImGui::Selectable(std::to_string(item->offset).c_str(), selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap | ImGuiSelectableFlags_AllowDoubleClick))
								{
									static auto last_click = &*item;
									if (ImGui::GetIO().KeyCtrl)
									{
										if (selected)
										{
											selection.erase(&*item);
										}
										else
										{
											selection.insert(&*item);
										}
									}
									else if (ImGui::GetIO().KeyShift)
									{
										if (last_click != &*item)
										{
											auto last = scan_addr.find(*last_click);
											auto left = &last, right = &item;
											if (**right < **left)
											{
												swap(left, right);
											}
											for_each(*left, *right, [](const auto& x)
											{
												selection.insert(&x);
											});
											selection.insert(&**right);
										}
									}
									else
									{
										selection.clear();
										selection.insert(&*item);
									}
									last_click = &*item;

									if (ImGui::IsMouseDoubleClicked(0))
									{
										for (const auto x : selection)
										{
											list_addr.emplace_back(x->offset, x->type);
										}
										selection.clear();
									}
								}

								if (ImGui::IsKeyPressed(ImGuiKey_Delete, false))
								{
									for (const auto x : selection)
									{
										scan_addr.erase(*x);
									}
									selection.clear();
									break;
								}
							}

							if (ImGui::TableSetColumnIndex(1))
							{
								ImGui::Text("%08llX", GTA5.get_global_addr(item->offset));
							}

							if (ImGui::TableSetColumnIndex(2))
							{
								switch (item->type)
								{
									case SCAN_BOOL:
									{
										ImGui::Text(GTA5.get_global <bool>(item->offset) ? "True" : "False");
										break;
									}
									case SCAN_FLOAT:
									{
										ImGui::Text("%f", static_cast <double>(GTA5.get_global <float>(item->offset)));
										break;
									}
									case SCAN_INT:
									{
										ImGui::Text("%d", GTA5.get_global <int>(item->offset));
										break;
									}
									case SCAN_UINT:
									{
										ImGui::Text("%u", GTA5.get_global <uint32_t>(item->offset));
										break;
									}
									case SCAN_STRING:
									{
										ImGui::Text("%s", GTA5.get_global <const char*>(item->offset));
										break;
									}
									case SCAN_NUMBER:
									case SCAN_NONE: ;
								}
							}

							if (ImGui::TableSetColumnIndex(3))
							{
								switch (item->type)
								{
									case SCAN_BOOL:
									{
										ImGui::Text(item->previous_value.bool_value ? "True" : "False");
										break;
									}
									case SCAN_FLOAT:
									{
										ImGui::Text("%f", static_cast <double>(item->previous_value.float_value));
										break;
									}
									case SCAN_INT:
									{
										ImGui::Text("%d", item->previous_value.int_value);
										break;
									}
									case SCAN_UINT:
									{
										ImGui::Text("%u", item->previous_value.uint_value);
										break;
									}
									case SCAN_STRING:
									{
										ImGui::Text("%s", item->previous_value.string_value);
										break;
									}
									case SCAN_NUMBER:
									case SCAN_NONE: ;
								}
							}
						}
					}
					ImGui::EndTable();
				}
			}
			ImGui::EndChild();
		}
		ImGui::EndChild();

		ImGui::Spacing();
		ImGui::BeginChild("Address List", ImVec2(0, list_height));
		{
			/*if (list_addr.empty())//debug
			{
				for (auto i = 1; i <= 5; i++)
				{
					list_addr.emplace_back(i * 8, SCAN_BOOL);
				}
			}*/

			if (ImGui::BeginTable("Address List", 5, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersV | ImGuiTableFlags_BordersH | ImGuiTableFlags_ScrollY))
			{
				ImGui::TableSetupColumn("Name");
				ImGui::TableSetupColumn("Offset");
				ImGui::TableSetupColumn("Address");
				ImGui::TableSetupColumn("Type");
				ImGui::TableSetupColumn("Value");
				ImGui::TableSetupScrollFreeze(0, 1);
				ImGui::TableHeadersRow();
				{
					static std::set <size_t> selection;
					for (size_t i = 0; i < list_addr.size(); i++)
					{
						const auto item     = list_addr.begin() + static_cast <ptrdiff_t>(i);
						const bool selected = selection.contains(i);

						ImGui::TableNextRow();

						if (ImGui::TableSetColumnIndex(0))
						{
							static auto popup_opened = static_cast <size_t>(-1);
							if (ImGui::Selectable(item->name, selected, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap | ImGuiSelectableFlags_AllowDoubleClick))
							{
								static auto last_click = i;
								if (ImGui::GetIO().KeyCtrl)
								{
									if (selected)
									{
										selection.erase(i);
									}
									else
									{
										selection.insert(i);
									}
								}
								else if (ImGui::GetIO().KeyShift)
								{
									if (last_click != i)
									{
										auto left = &last_click, right = &i;
										if (*right < *left)
										{
											std::swap(left, right);
										}
										for (auto j = *left; j <= *right; j++)
										{
											selection.insert(j);
										}
									}
								}
								else
								{
									selection.clear();
									selection.insert(i);
								}
								last_click = i;

								if (ImGui::IsMouseDoubleClicked(0))
								{
									popup_opened = i;
								}
							}

							if (bool opened = popup_opened == i; opened)
							{
								const auto window = ImGui::GetCurrentWindow()->ParentWindow->ParentWindow;
								ImGui::SetNextWindowPos(window->Pos + window->Size / 2, ImGuiCond_FirstUseEver, ImVec2(0.5f, 0.5f));
								if (ImGui::Begin(("Edit Item##" + std::to_string(i)).c_str(), &opened, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize))
								{
									static scan_item  unsaved = *item;
									static scan_value unsaved_value;
									static auto       new_unsaved = true;
									if (new_unsaved)
									{
										unsaved = *item;
										switch (item->type)
										{
											case SCAN_BOOL:
											{
												unsaved_value = scan_value(GTA5.get_global <bool>(item->offset));
												break;
											}
											case SCAN_FLOAT:
											{
												unsaved_value = scan_value(GTA5.get_global <float>(item->offset));
												break;
											}
											case SCAN_INT:
											{
												unsaved_value = scan_value(GTA5.get_global <int>(item->offset));
												break;
											}
											case SCAN_UINT:
											{
												unsaved_value = scan_value(GTA5.get_global <uint32_t>(item->offset));
												break;
											}
											case SCAN_STRING:
											{
												unsaved_value = scan_value(GTA5.get_global <const char*>(item->offset));
												break;
											}
											case SCAN_NUMBER:
											case SCAN_NONE: ;
										}
										new_unsaved = false;
									}

									ImGui::Text("Name");
									{
										ImGui::InputText("Name", unsaved.name, MAX_NAME_LEN);
										ImGui::SameLine();
										ImGui::Text("Length: %lld", std::strlen(unsaved.name));
										if (std::strlen(unsaved.name) == MAX_NAME_LEN - 1)
										{
											ImGui::Text("Warning: Max name length reached. (%d)", MAX_NAME_LEN - 1);
										}
									}

									ImGui::Text("Type");
									{
										const char* items[]  = { "Bool", "Float", "Int", "UInt", "String" };
										auto        cur_item = -1;
										for (int x = unsaved.type; x; cur_item++)
										{
											x = x >> 1;
										}
										ImGui::Combo("Type", &cur_item, items,IM_ARRAYSIZE(items));
										unsaved.type = static_cast <scan_type>(1 << cur_item);
									}

									ImGui::Text("Value");
									{
										switch (unsaved.type)
										{
											case SCAN_BOOL:
											{
												int bool_value = unsaved_value.bool_value;
												ImGui::RadioButton("True", &bool_value, true);
												ImGui::SameLine();
												ImGui::RadioButton("False", &bool_value, false);
												unsaved_value.bool_value = bool_value;
												break;
											}
											case SCAN_FLOAT:
											{
												auto x = static_cast <double>(unsaved_value.float_value);
												ImGui::InputDouble("Value", &x);
												unsaved_value = scan_value(x);
												break;
											}
											case SCAN_INT:
											{
												int x = unsaved_value.int_value;
												ImGui::InputInt("Value", &x, 0);
												unsaved_value = scan_value(x);
												break;
											}
											case SCAN_UINT:
											{
												uint32_t x = unsaved_value.uint_value;
												ImGui::InputScalar("Value", ImGuiDataType_U32, &x);
												unsaved_value = scan_value(x);
												break;
											}
											case SCAN_STRING:
											{
												unsaved_value.reset(unsaved_value.string_value);
												ImGui::InputText("Value", unsaved_value.string_value, MAX_STR_LEN);
												ImGui::SameLine();
												ImGui::Text("Length: %lld", strlen(unsaved_value.string_value));
												if (strlen(unsaved_value.string_value) == static_cast <size_t>(MAX_STR_LEN - 1))
												{
													ImGui::Text("Warning: Max string length reached. (%d)", MAX_STR_LEN - 1);
												}
												break;
											}
											case SCAN_NUMBER:
											case SCAN_NONE: ;
										}
									}

									ImGui::SetItemDefaultFocus();
									if (ImGui::Button("OK", ImVec2(120, 0)))
									{
										*item = unsaved;
										switch (item->type)
										{
											case SCAN_BOOL:
											{
												GTA5.set_global <bool>(item->offset, unsaved_value.bool_value);
												break;
											}
											case SCAN_FLOAT:
											{
												GTA5.set_global <float>(item->offset, unsaved_value.float_value);
												break;
											}
											case SCAN_INT:
											{
												GTA5.set_global <int>(item->offset, unsaved_value.int_value);
												break;
											}
											case SCAN_UINT:
											{
												GTA5.set_global <uint32_t>(item->offset, unsaved_value.uint_value);
												break;
											}
											case SCAN_STRING:
											{
												strcpy_s(reinterpret_cast <char*>(GTA5.get_global_addr(item->offset)), strlen(unsaved_value.string_value), unsaved_value.string_value);
												break;
											}
											case SCAN_NUMBER:
											case SCAN_NONE: ;
										}
										new_unsaved = true;
										opened      = false;
									}
									ImGui::SameLine();
									if (ImGui::Button("Cancel", ImVec2(120, 0)))
									{
										new_unsaved = true;
										opened      = false;
									}
									ImGui::End();
								}
								if (!opened)
								{
									popup_opened = static_cast <size_t>(-1);
								}
							}
							else
							{
								GImGui->NextWindowData.ClearFlags();

								if (popup_opened == static_cast <size_t>(-1) && ImGui::IsKeyPressed(ImGuiKey_Delete, false))
								{
									for (const auto it : std::ranges::reverse_view(selection))
									{
										list_addr.erase(list_addr.begin() + static_cast <ptrdiff_t>(it));
									}
									selection.clear();
									break;
								}
							}
						}

						if (ImGui::TableSetColumnIndex(1))
						{
							ImGui::Text("%d", item->offset);
						}

						if (ImGui::TableSetColumnIndex(2))
						{
							ImGui::Text("%08llX", GTA5.get_global_addr(item->offset));
						}

						if (ImGui::TableSetColumnIndex(3))
						{
							ImGui::Text("%s", to_string(item->type).c_str());
						}

						if (ImGui::TableSetColumnIndex(4))
						{
							switch (item->type)
							{
								case SCAN_BOOL:
								{
									ImGui::Text(GTA5.get_global <bool>(item->offset) ? "True" : "False");
									break;
								}
								case SCAN_FLOAT:
								{
									ImGui::Text("%f", static_cast <double>(GTA5.get_global <float>(item->offset)));
									break;
								}
								case SCAN_INT:
								{
									ImGui::Text("%d", GTA5.get_global <int>(item->offset));
									break;
								}
								case SCAN_UINT:
								{
									ImGui::Text("%u", GTA5.get_global <uint32_t>(item->offset));
									break;
								}
								case SCAN_STRING:
								{
									ImGui::Text("%s", GTA5.get_global <const char*>(item->offset));
									break;
								}
								case SCAN_NUMBER:
								case SCAN_NONE: ;
							}
						}
					}
				}
				ImGui::EndTable();
			}
		}
		ImGui::EndChild();
	}
	ImGui::End();
}

template <typename T, scan_type ST> void ScanGlobalLocal(std::set <scan_result>& s, T value)
{
	if (s.empty())
	{
		// TODO
	}
	else
	{
		for (const auto& result : s)
		{
			if (result.type == ST)
			{
				if (equal(GTA5.get_global <T>(result.offset), value))
				{
					s.erase(result);
				}
			}
		}
	}
}

template <> void ScanGlobalLocal <const char*, SCAN_STRING>(std::set <scan_result>& s, const char* value)
{
	if (s.empty())
	{
		//TODO
		(void) s;
	}
	else
	{
		for (const auto& result : s)
		{
			if (result.type == SCAN_STRING)
			{
				if (!strcmp(GTA5.get_global <const char*>(result.offset), value))
				{
					s.erase(result);
				}
			}
		}
	}
}

void ScanGlobalLocal(std::set <scan_result>& s, const int scan_type, const scan_value& value)
{
	if (scan_type & SCAN_BOOL)
	{
		ScanGlobalLocal <bool, SCAN_BOOL>(s, value.bool_value);
	}
	else if (scan_type & SCAN_STRING)
	{
		ScanGlobalLocal <const char*, SCAN_STRING>(s, value.string_value);
	}
	else
	{
		if (scan_type & SCAN_FLOAT)
		{
			ScanGlobalLocal <float, SCAN_FLOAT>(s, value.float_value);
		}
		if (scan_type & SCAN_INT)
		{
			ScanGlobalLocal <int, SCAN_INT>(s, value.int_value);
		}
		if (scan_type & SCAN_UINT)
		{
			ScanGlobalLocal <uint32_t, SCAN_UINT>(s, value.uint_value);
		}
	}
}
