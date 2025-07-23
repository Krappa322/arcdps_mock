// This file is derived from the example in imgui/examples/example_win32_directx9/main.cpp

#include "CombatMock.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx9.h>
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_impl_win32.h>

#include <magic_enum/magic_enum.hpp>

#include <d3d9.h>
#include <d3d11.h>
#include <iostream>
#include <memory>
#include <tchar.h>
#include <filesystem>

#include <ArcdpsExtension/arcdps_structs.h>


extern "C" __declspec(dllexport) const wchar_t* e0();
extern "C" __declspec(dllexport) void e3(const char* pString);
extern "C" __declspec(dllexport) void e5(ImVec4** colors);
extern "C" __declspec(dllexport) uint64_t e6();
extern "C" __declspec(dllexport) uint64_t e7();
extern "C" __declspec(dllexport) void e8(const char* pString);
extern "C" __declspec(dllexport) void e9(cbtevent* pEvent, uint32_t pSignature);

// Data
static int						g_DxMode = 9;
static LPDIRECT3D9              g_pD3D9 = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice9 = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

static ID3D11Device*            g_pd3d11Device = NULL;
static ID3D11DeviceContext*     g_pd3d11DeviceContext = NULL;
static IDXGISwapChain*          g_pSwapChain = NULL;
static ID3D11RenderTargetView*  g_mainRenderTargetView = NULL;

static ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
bool CreateDeviceD3D9(HWND hWnd);
bool CreateDeviceD3D11(HWND hWnd);
void CreateRenderTarget();
void CleanupDeviceD3D();
void CleanupDeviceD3D9();
void CleanupDeviceD3D11();
void CleanupRenderTarget();
void ResetDevice();
void RenderFrame();
void RenderFrame9();
void RenderFrame11();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

static std::wstring e0ConfigPath;
static arcdps_exports TEST_MODULE_EXPORTS;
static std::unique_ptr<CombatMock> combatMock;

namespace
{
#pragma pack(push, 1)
struct ArcModifiers
{
	uint16_t _1 = VK_SHIFT;
	uint16_t _2 = VK_MENU;
	uint16_t Multi = 0;
	uint16_t Fill = 0;
};
#pragma pack(pop)
}

// arcdps config path
const wchar_t* e0() 
{
	return e0ConfigPath.c_str();
}

// arcdps file log
void e3(const char* pString)
{
	if (!combatMock) {
		return;
	}
	combatMock->e3LogLine(pString);
}

/* e5 writes out colour array ptrs, sizeof(out) == sizeof(ImVec4*) * 5.  [ void e5(ImVec4** out) ]
	  out[0] = core cols
				  enum n_colours_core {
					CCOL_TRANSPARENT,
					CCOL_WHITE,
					CCOL_LWHITE,
					CCOL_LGREY,
					CCOL_LYELLOW,
					CCOL_LGREEN,
					CCOL_LRED,
					CCOL_LTEAL,
					CCOL_MGREY,
					CCOL_DGREY,
					CCOL_NUM
				  };
	  out[1] = prof colours base
	  out[2] = prof colours highlight
				  prof colours match prof enum, 0 unknown
	  out[3] = subgroup colours base
	  out[4] = subgroup colours highlight
				  subgroup colours match subgroup, up to game max, out[3][15]
	*/
static std::array coreCols { // size of enum `n_colours_core`
	ImVec4(1.000000f,1.000000f,1.000000f,0.000000f),
	ImVec4(1.000000f,1.000000f,1.000000f,1.000000f),
	ImVec4(0.800000f,0.800000f,0.830000f,1.000000f),
	ImVec4(0.690000f,0.650000f,0.660000f,1.000000f),
	ImVec4(1.000000f,1.000000f,0.380000f,1.000000f),
	ImVec4(0.380000f,1.000000f,0.380000f,1.000000f),
	ImVec4(1.000000f,0.380000f,0.380000f,1.000000f),
	ImVec4(0.380000f,1.000000f,1.000000f,1.000000f),
	ImVec4(0.500000f,0.470000f,0.480000f,1.000000f),
	ImVec4(0.250000f,0.220000f,0.230000f,1.000000f),
	ImVec4(0.000000f,0.000000f,5.688562f,-0.000000f)
};
static_assert(coreCols.size() == magic_enum::enum_count<ColorsCore>());
static std::array profColsBase { // size of enum `Prof`
	ImVec4(0.340000f,0.300000f,0.360000f,0.490000f),
	ImVec4(0.040000f,0.870000f,1.000000f,0.430000f),
	ImVec4(1.000000f,0.830000f,0.240000f,0.430000f),
	ImVec4(0.890000f,0.450000f,0.160000f,0.430000f),
	ImVec4(0.530000f,0.870000f,0.040000f,0.430000f),
	ImVec4(0.890000f,0.370000f,0.450000f,0.450000f),
	ImVec4(0.970000f,0.220000f,0.220000f,0.430000f),
	ImVec4(0.800000f,0.230000f,0.820000f,0.430000f),
	ImVec4(0.020000f,0.890000f,0.490000f,0.430000f),
	ImVec4(0.630000f,0.160000f,0.160000f,0.450000f)
};
static_assert(profColsBase.size() == magic_enum::enum_count<Prof>());
static std::array profColsHighlight { // size of enum `Prof`
	ImVec4(0.340000f,0.300000f,0.360000f,0.250000f),
	ImVec4(0.040000f,0.870000f,1.000000f,0.210000f),
	ImVec4(1.000000f,0.830000f,0.240000f,0.210000f),
	ImVec4(0.890000f,0.450000f,0.160000f,0.210000f),
	ImVec4(0.530000f,0.870000f,0.040000f,0.210000f),
	ImVec4(0.890000f,0.370000f,0.450000f,0.280000f),
	ImVec4(0.970000f,0.220000f,0.220000f,0.210000f),
	ImVec4(0.800000f,0.230000f,0.820000f,0.210000f),
	ImVec4(0.020000f,0.890000f,0.490000f,0.210000f),
	ImVec4(0.630000f,0.160000f,0.160000f,0.280000f)
};
static_assert(profColsHighlight.size() == magic_enum::enum_count<Prof>());
static std::array subgroupColsBase { // max amount of subgroups (currently 15), defined by gw2
	ImVec4(0.340000f,0.300000f,0.360000f,0.490000f),
	ImVec4(0.970000f,0.140000f,0.140000f,0.430000f),
	ImVec4(0.140000f,0.450000f,0.970000f,0.430000f),
	ImVec4(0.640000f,0.140000f,0.970000f,0.430000f),
	ImVec4(0.140000f,0.970000f,0.970000f,0.430000f),
	ImVec4(0.970000f,0.970000f,0.140000f,0.430000f),
	ImVec4(0.970000f,0.470000f,0.140000f,0.430000f),
	ImVec4(0.140000f,0.970000f,0.140000f,0.430000f),
	ImVec4(0.970000f,0.140000f,0.970000f,0.430000f),
	ImVec4(0.470000f,0.400000f,0.190000f,0.430000f),
	ImVec4(1.000000f,0.140000f,0.640000f,0.430000f),
	ImVec4(0.800000f,1.000000f,0.000000f,0.430000f),
	ImVec4(1.000000f,0.800000f,0.000000f,0.430000f),
	ImVec4(0.000000f,0.800000f,1.000000f,0.430000f),
	ImVec4(1.000000f,0.470000f,0.800000f,0.430000f)
};
static_assert(subgroupColsBase.size() == 15);
static std::array subgroupColsHighlight { // max amount of subgroups (currently 15), defined by gw2
	ImVec4(0.340000f,0.300000f,0.360000f,0.250000f),
	ImVec4(0.970000f,0.140000f,0.140000f,0.210000f),
	ImVec4(0.140000f,0.450000f,0.970000f,0.210000f),
	ImVec4(0.640000f,0.140000f,0.970000f,0.210000f),
	ImVec4(0.140000f,0.970000f,0.970000f,0.210000f),
	ImVec4(0.970000f,0.970000f,0.140000f,0.210000f),
	ImVec4(0.970000f,0.470000f,0.140000f,0.210000f),
	ImVec4(0.140000f,0.970000f,0.140000f,0.210000f),
	ImVec4(0.970000f,0.140000f,0.970000f,0.210000f),
	ImVec4(0.470000f,0.400000f,0.190000f,0.210000f),
	ImVec4(1.000000f,0.140000f,0.640000f,0.210000f),
	ImVec4(0.800000f,1.000000f,0.000000f,0.210000f),
	ImVec4(1.000000f,0.800000f,0.000000f,0.210000f),
	ImVec4(0.000000f,0.800000f,1.000000f,0.210000f),
	ImVec4(1.000000f,0.470000f,0.800000f,0.210000f)
};
static_assert(subgroupColsHighlight.size() == 15);

void e5(ImVec4** colors)
{
	colors[0] = coreCols.data();
	colors[1] = profColsBase.data();
	colors[2] = profColsHighlight.data();
	colors[3] = subgroupColsBase.data();
	colors[4] = subgroupColsHighlight.data();
}

uint64_t e6()
{
	return 0; // everything set to false
}

uint64_t e7()
{
	ArcModifiers mods;
	return *reinterpret_cast<uint64_t*>(&mods);
}

void e8(const char* pString)
{
	if (!combatMock) {
		return;
	}
	if (!pString) {
		combatMock->e8LogLine("(null)");
	}
	else 
	{
		combatMock->e8LogLine(pString);
	}
}

void e9(cbtevent*, uint32_t)
{
	return; // Ignore evtc log from addon
}

const char* MOCK_VERSION = "20210909.ARCDPS_MOCK.0.1";


int Run(const char* pModulePath, const char* pMockFilePath, const char* pSelfPath)
{
	// Create application window
	//ImGui_ImplWin32_EnableDpiAwareness();
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("Arcdps Mock"), NULL };
	::RegisterClassEx(&wc);
	HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("Arcdps Mock"), WS_OVERLAPPEDWINDOW | WS_MAXIMIZE, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

	// Initialize Direct3D
	if (!CreateDeviceD3D(hwnd))
	{
		CleanupDeviceD3D();
		::UnregisterClass(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	// Show the window
	::ShowWindow(hwnd, SW_MAXIMIZE);
	::UpdateWindow(hwnd);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	//ImGui::StyleColorsDark();
	ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd);
	if (g_DxMode == 9)
		ImGui_ImplDX9_Init(g_pd3dDevice9);
	if (g_DxMode == 11)
		ImGui_ImplDX11_Init(g_pd3d11Device, g_pd3d11DeviceContext);

	// Load Fonts
	// - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
	// - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
	// - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
	// - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
	// - Read 'docs/FONTS.md' for more instructions and details.
	// - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
	//io.Fonts->AddFontDefault();
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
	//io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
	//ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
	//IM_ASSERT(font != NULL);

	HMODULE selfHandle = GetModuleHandle(NULL);
	HMODULE testModuleHandle = LoadLibraryA(pModulePath);
	if (testModuleHandle == NULL)
	{
		fprintf(stderr, "Failed loading '%s' - %u\n", pModulePath, GetLastError());
		assert(false);
	}

	auto get_init_addr = reinterpret_cast<GetInitAddrSignature>(GetProcAddress(testModuleHandle, "get_init_addr"));
	assert(get_init_addr != nullptr);
	auto get_release_addr = reinterpret_cast<GetReleaseAddrSignature>(GetProcAddress(testModuleHandle, "get_release_addr"));
	assert(get_release_addr != nullptr);

	ModInitSignature mod_init = get_init_addr(MOCK_VERSION, ImGui::GetCurrentContext(), (g_DxMode == 9) ? (void*)g_pd3dDevice9 : (void*)g_pSwapChain, selfHandle, malloc, free, g_DxMode);
	assert(mod_init != nullptr);
	arcdps_exports* temp_exports = mod_init();
	memcpy(&TEST_MODULE_EXPORTS, temp_exports, sizeof(TEST_MODULE_EXPORTS)); // Maybe do some deep copy at some point but we're not using the strings in there anyways

	if (temp_exports->sig == 0) {
		std::cout << (char*)temp_exports->size << std::endl;
		assert(false);
	}
	
	combatMock = std::make_unique<CombatMock>(&TEST_MODULE_EXPORTS);
	if (pMockFilePath != nullptr)
	{
		uint32_t result = combatMock->LoadFromFile(pMockFilePath);
		assert(result == 0);
		combatMock->Execute();
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 4.f, 4.f });
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, { 5.f, 3.f });
	ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 0.f);
	ImGui::PushStyleVar(ImGuiStyleVar_PopupBorderSize, 0.f);
	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4.f, 4.f });
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 5.f, 3.f });
	ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, { 5.f, 3.f });
	ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, 25.f);
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, 9.f);
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0.f);
	ImGui::PushStyleVar(ImGuiStyleVar_GrabMinSize, 25.f);
	ImGui::PushStyleVar(ImGuiStyleVar_TabRounding, 0.f);

	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 0.8f, 0.83f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0.24f, 0.23f, 0.29f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.05f, 0.07f, 0.75f));
	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.07f, 0.07f, 0.09f, 0.f));
	ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.07f, 0.07f, 0.09f, 0.85f));
	ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.64f, 0.57f, 0.7f, 0.2f));
	ImGui::PushStyleColor(ImGuiCol_BorderShadow, ImVec4(0.64f, 0.62f, 0.67f, 0.f));
	ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.62f, 0.6f, 0.65f, 0.2f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.62f, 0.6f, 0.65f, 0.75f));
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.56f, 0.56f, 0.58f, 0.75f));
	ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.1f, 0.09f, 0.12f, 0.85f));
	ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.1f, 0.09f, 0.12f, 0.85f));
	ImGui::PushStyleColor(ImGuiCol_TitleBgCollapsed, ImVec4(0.1f, 0.09f, 0.12f, 0.85f));
	ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.1f, 0.09f, 0.12f, 0.7f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ImVec4(0.1f, 0.09f, 0.12f, 0.8f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ImVec4(0.46f, 0.45f, 0.47f, 0.78f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ImVec4(0.67f, 0.67f, 0.69f, 0.78f));
	ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ImVec4(0.78f, 0.78f, 0.8f, 0.78f));
	ImGui::PushStyleColor(ImGuiCol_CheckMark, ImVec4(0.8f, 0.8f, 0.83f, 0.81f));
	ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0.8f, 0.8f, 0.83f, 0.31f));
	ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0.06f, 0.05f, 0.07f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.62f, 0.6f, 0.65f, 0.3f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.62f, 0.6f, 0.65f, 0.6f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.62f, 0.6f, 0.65f, 0.9f));
	ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.36f, 0.36f, 0.38f, 0.7f));
	ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.36f, 0.36f, 0.38f, 0.35f));
	ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.36f, 0.36f, 0.38f, 0.7f));
	ImGui::PushStyleColor(ImGuiCol_ResizeGrip, ImVec4(0.f, 0.f, 0.f, 0.f));
	ImGui::PushStyleColor(ImGuiCol_ResizeGripHovered, ImVec4(0.56f, 0.56f, 0.58f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_ResizeGripActive, ImVec4(0.06f, 0.05f, 0.07f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_Tab, ImVec4(0.7f, 0.68f, 0.69f, 0.1f));
	ImGui::PushStyleColor(ImGuiCol_TabHovered, ImVec4(0.7f, 0.68f, 0.69f, 0.3f));
	ImGui::PushStyleColor(ImGuiCol_TabActive, ImVec4(0.7f, 0.68f, 0.69f, 0.43f));
	ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0.7f, 0.68f, 0.66f, 0.56f));
	ImGui::PushStyleColor(ImGuiCol_PlotLinesHovered, ImVec4(0.25f, 1.f, 0.f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.7f, 0.68f, 0.66f, 0.48f));
	ImGui::PushStyleColor(ImGuiCol_PlotHistogramHovered, ImVec4(0.25f, 1.f, 0.f, 1.f));
	ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, ImVec4(0.36f, 0.36f, 0.88f, 0.55f));

	// Main loop
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	while (msg.message != WM_QUIT)
	{
		// Poll and handle messages (inputs, window resize, etc.)
		// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
		// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
		// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
		// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
		if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			continue;
		}

		// Start the Dear ImGui frame
		if (g_DxMode == 9)
			ImGui_ImplDX9_NewFrame();
		if (g_DxMode == 11)
			ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// Show options window (mirror of arcdps options)
		{
			ImGui::Begin("Options", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize);

			if (ImGui::BeginTabBar("tabBar"))
			{
				if (ImGui::BeginTabItem("Interface")) 
				{
					ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.f, 0.f });

					ImGui::TextDisabled("Windows");

					if (TEST_MODULE_EXPORTS.options_windows != nullptr) 
					{
						TEST_MODULE_EXPORTS.options_windows("squad");
						TEST_MODULE_EXPORTS.options_windows("dps");
						TEST_MODULE_EXPORTS.options_windows("skills");
						TEST_MODULE_EXPORTS.options_windows("metrics");
						TEST_MODULE_EXPORTS.options_windows("error");
						TEST_MODULE_EXPORTS.options_windows(nullptr);
					}

					ImGui::PopStyleVar();

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Extensions"))
				{
					if (ImGui::BeginTabBar("ExtensionsTabBar"))
					{
						if (ImGui::BeginTabItem(TEST_MODULE_EXPORTS.out_name))
						{
							if (TEST_MODULE_EXPORTS.options_end != nullptr)
							{
								TEST_MODULE_EXPORTS.options_end();
							}

							ImGui::EndTabItem();
						}

						ImGui::EndTabBar();
					}

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("About"))
				{
					ImGui::Text("arcdps_mock");

					ImGui::PushStyleColor(ImGuiCol_TextDisabled, 0xFFA3A3A3);

					ImGui::Indent();
					ImGui::TextDisabled("path:");
					ImGui::Indent();
					ImGui::TextDisabled("%s", pSelfPath);
					ImGui::Unindent();
					ImGui::TextDisabled("mock file:");
					ImGui::Indent();
					ImGui::TextDisabled("%s", (pMockFilePath != nullptr) ? pMockFilePath : "None");
					ImGui::Unindent();
					ImGui::Checkbox("Logs", &combatMock->showLog);
					ImGui::Unindent();

					ImGui::Text("extension");
					ImGui::Indent();
					ImGui::TextDisabled("%s: %s", TEST_MODULE_EXPORTS.out_name, TEST_MODULE_EXPORTS.out_build);
					ImGui::Indent();
					ImGui::TextDisabled("%s", pModulePath);
					ImGui::Unindent();
					ImGui::Unindent();

					ImGui::Text("directx");
					ImGui::Indent();
					ImGui::TextDisabled("%d", g_DxMode);
					ImGui::Unindent();

					ImGui::PopStyleColor();

					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}

			ImGui::End();
		}

		combatMock->DisplayWindow();

		if (TEST_MODULE_EXPORTS.imgui != nullptr)
		{
			TEST_MODULE_EXPORTS.imgui(static_cast<uint32_t>(true), static_cast<uint32_t>(false));
		}

		// Rendering
		ImGui::EndFrame();

		RenderFrame();
	}

	ModReleaseSignature mod_release = reinterpret_cast<ModReleaseSignature>(get_release_addr());
	assert(mod_release != nullptr);
	mod_release();

	if (g_DxMode == 9)
		ImGui_ImplDX9_Shutdown();
	if (g_DxMode == 11)
		ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();

	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(hwnd);
	::UnregisterClass(wc.lpszClassName, wc.hInstance);

	return 0;
}

// Main code
int main(int pArgumentCount, const char** pArgumentVector)
{
	const char* modulePath = nullptr;
	const char* mockFilePath = nullptr;
	const char* selfPathString = pArgumentVector[0];

	int num = 0;
	// start with 1, first value is always the executable
	for (int i = 1; i < pArgumentCount; ++i)
	{
		const char* string = pArgumentVector[i];

		if (std::string(string) == "-dx11")
		{
			g_DxMode = 11;
			continue;
		}
		if (std::string(string) == "-dx9")
		{
			g_DxMode = 9;
			continue;
		}
		if (num == 0)
		{
			modulePath = string;
			++num;
			continue;
		}
		if (num == 1)
		{
			mockFilePath = string;
			++num;
			continue;
		}
	}

	if (num == 0) 
	{
		assert(false && "Too few arguments sent");
		return 1;
	}

	// create arcdps settings directoy
	if (!(CreateDirectoryA("addons\\", NULL) || ERROR_ALREADY_EXISTS == GetLastError())) {
		assert(false && "Error creating addons directory");
	}
	if (!(CreateDirectoryA("addons\\arcdps\\", NULL) || ERROR_ALREADY_EXISTS == GetLastError())) {
		assert(false && "Error creating arcdps directory");
	}

	std::filesystem::path selfPath(selfPathString);
	selfPath.remove_filename();
	selfPath.append("addons");
	selfPath.append("arcdps");
	selfPath.append("arcdps.ini");
	e0ConfigPath = selfPath.wstring();

	return Run(modulePath, mockFilePath, selfPathString);
}

// Helper functions
bool CreateDeviceD3D(HWND hWnd)
{
	if (g_DxMode == 9)
	{
		return CreateDeviceD3D9(hWnd);
	}
	if (g_DxMode == 11)
	{
		return CreateDeviceD3D11(hWnd);
	}
	throw std::runtime_error("tried to create invalid dx mode device");
}

bool CreateDeviceD3D9(HWND hWnd)
{
	if ((g_pD3D9 = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
		return false;

	// Create the D3DDevice
	ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
	g_d3dpp.Windowed = TRUE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	g_d3dpp.EnableAutoDepthStencil = TRUE;
	g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
	//g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
	if (g_pD3D9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice9) < 0)
		return false;

	return true;
}

bool CreateDeviceD3D11(HWND hWnd)
{
	// Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3d11Device, &featureLevel, &g_pd3d11DeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CreateRenderTarget()
{
	if (g_DxMode == 11)
	{
		ID3D11Texture2D* pBackBuffer;
		g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
		g_pd3d11Device->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
		pBackBuffer->Release();
	}
}

void CleanupDeviceD3D()
{
	if (g_DxMode == 9)
	{
		CleanupDeviceD3D9();
	}
	else if (g_DxMode == 11)
	{
		CleanupDeviceD3D11();
	}
}

void CleanupDeviceD3D9()
{
	if (g_pd3dDevice9) { g_pd3dDevice9->Release(); g_pd3dDevice9 = NULL; }
	if (g_pD3D9) { g_pD3D9->Release(); g_pD3D9 = NULL; }
}

void CleanupDeviceD3D11()
{
	CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3d11DeviceContext) { g_pd3d11DeviceContext->Release(); g_pd3d11DeviceContext = NULL; }
    if (g_pd3d11Device) { g_pd3d11Device->Release(); g_pd3d11Device = NULL; }
}

void CleanupRenderTarget()
{
	if (g_DxMode == 11)
	{
		if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
	}
}

void ResetDevice()
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	HRESULT hr = g_pd3dDevice9->Reset(&g_d3dpp);
	if (hr == D3DERR_INVALIDCALL)
		IM_ASSERT(0);
	ImGui_ImplDX9_CreateDeviceObjects();
}

void RenderFrame()
{
	if (g_DxMode == 9)
	{
		RenderFrame9();
	}
	else if (g_DxMode == 11)
	{
		RenderFrame11();
	}
}

void RenderFrame9()
{
	g_pd3dDevice9->SetRenderState(D3DRS_ZENABLE, FALSE);
	g_pd3dDevice9->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	g_pd3dDevice9->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
	
	D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x * 255.0f), (int)(clear_color.y * 255.0f), (int)(clear_color.z * 255.0f), (int)(clear_color.w * 255.0f));
	g_pd3dDevice9->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
	if (g_pd3dDevice9->BeginScene() >= 0)
	{
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		g_pd3dDevice9->EndScene();
	}
	HRESULT result = g_pd3dDevice9->Present(NULL, NULL, NULL, NULL);

	// Handle loss of D3D9 device
	if (result == D3DERR_DEVICELOST && g_pd3dDevice9->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		ResetDevice();
}

void RenderFrame11()
{
	ImGui::Render();
	const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
    g_pd3d11DeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
    g_pd3d11DeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    g_pSwapChain->Present(1, 0); // Present with vsync
    //g_pSwapChain->Present(0, 0); // Present without vsync
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (TEST_MODULE_EXPORTS.wnd_nofilter != nullptr)
	{
		msg = TEST_MODULE_EXPORTS.wnd_nofilter(hWnd, msg, wParam, lParam);
		if (msg == 0)
		{
			return 0;
		}
	}

	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (g_DxMode == 9 && g_pd3dDevice9 != NULL && wParam != SIZE_MINIMIZED)
		{
			g_d3dpp.BackBufferWidth = LOWORD(lParam);
			g_d3dpp.BackBufferHeight = HIWORD(lParam);
			ResetDevice();
		}
		if (g_DxMode == 11 && g_pd3d11Device != NULL && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}

	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}