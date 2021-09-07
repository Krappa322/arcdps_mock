// This file is derived from the example in imgui/examples/example_win32_directx11/main.cpp

#include "CombatMock.h"

#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_dx11.h"
#include "../imgui/backends/imgui_impl_win32.h"

#include <d3d11.h>
#include <iostream>
#include <memory>
#include <tchar.h>

#include "arcdps_structs.h"


extern "C" __declspec(dllexport) void e3(const char* pString);
extern "C" __declspec(dllexport) void e5(ImVec4** colors);
extern "C" __declspec(dllexport) uint64_t e6();
extern "C" __declspec(dllexport) uint64_t e7();
extern "C" __declspec(dllexport) void e8(const char* pString);
extern "C" __declspec(dllexport) void e9(cbtevent* pEvent, uint32_t pSignature);

// Data
static ID3D11Device*            g_pd3dDevice = NULL;
static ID3D11DeviceContext*     g_pd3dDeviceContext = NULL;
static IDXGISwapChain*          g_pSwapChain = NULL;
static ID3D11RenderTargetView*  g_mainRenderTargetView = NULL;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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

// arcdps file log
void e3(const char* pString)
{
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
				  prof colours match prof enum
	  out[3] = subgroup colours base
	  out[4] = subgroup colours highlight
				  subgroup colours match subgroup, up to game max, out[3][15]
	*/
static ImVec4 coreCols[CCOL_FINAL_ENTRY] { // size of enum `n_colours_core`
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
static ImVec4 profColsBase[PROF_FINAL_ENTRY] { // size of enum `Prof`
	ImVec4(0.340000f,0.300000f,0.360000f,0.490000f),
	ImVec4(0.040000f,0.870000f,1.000000f,0.430000f),
	ImVec4(1.000000f,0.830000f,0.240000f,0.430000f),
	ImVec4(0.890000f,0.450000f,0.160000f,0.430000f),
	ImVec4(0.530000f,0.870000f,0.040000f,0.430000f),
	ImVec4(0.890000f,0.370000f,0.450000f,0.450000f),
	ImVec4(0.970000f,0.220000f,0.220000f,0.430000f),
	ImVec4(0.800000f,0.230000f,0.820000f,0.430000f),
	ImVec4(0.020000f,0.890000f,0.490000f,0.430000f)
};
static ImVec4 profColsHighlight[PROF_FINAL_ENTRY] { // size of enum `Prof`
	ImVec4(0.340000f,0.300000f,0.360000f,0.250000f),
	ImVec4(0.040000f,0.870000f,1.000000f,0.210000f),
	ImVec4(1.000000f,0.830000f,0.240000f,0.210000f),
	ImVec4(0.890000f,0.450000f,0.160000f,0.210000f),
	ImVec4(0.530000f,0.870000f,0.040000f,0.210000f),
	ImVec4(0.890000f,0.370000f,0.450000f,0.280000f),
	ImVec4(0.970000f,0.220000f,0.220000f,0.210000f),
	ImVec4(0.800000f,0.230000f,0.820000f,0.210000f),
	ImVec4(0.020000f,0.890000f,0.490000f,0.210000f)
};
static ImVec4 subgroupColsBase[15] { // max amount of subgroups (currently 15), defined by gw2
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
static ImVec4 subgroupColsHighlight[15] { // max amount of subgroups (currently 15), defined by gw2
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

void e5(ImVec4** colors)
{
	colors[0] = coreCols;
	colors[1] = profColsBase;
	colors[2] = profColsHighlight;
	colors[3] = subgroupColsBase;
	colors[4] = subgroupColsHighlight;
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
	combatMock->e8LogLine(pString);
}

void e9(cbtevent*, uint32_t)
{
	return; // Ignore evtc log from addon
}

const char* MOCK_VERSION = "ARCDPS_MOCK 0.1";


int Run(const char* pModulePath, const char* pMockFilePath)
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
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

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
		fprintf(stderr, "Failed loading '%s' - %u", pModulePath, GetLastError());
		assert(false);
	}

	auto get_init_addr = reinterpret_cast<GetInitAddrSignature>(GetProcAddress(testModuleHandle, "get_init_addr"));
	assert(get_init_addr != nullptr);
	auto get_release_addr = reinterpret_cast<GetReleaseAddrSignature>(GetProcAddress(testModuleHandle, "get_release_addr"));
	assert(get_release_addr != nullptr);

	ID3D11Device* pDevice;
	g_pSwapChain->GetDevice( __uuidof(pDevice), (void**)&pDevice);

	ModInitSignature mod_init = reinterpret_cast<ModInitSignature>(get_init_addr(MOCK_VERSION, ImGui::GetCurrentContext(), g_pSwapChain, selfHandle, malloc, free, 11));
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
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.f, 0.f });

		// Show options window (mirror of arcdps options)
		{
			ImGui::Begin("Options", nullptr, ImGuiWindowFlags_NoCollapse);

			if (TEST_MODULE_EXPORTS.options_windows != nullptr) {
				TEST_MODULE_EXPORTS.options_windows("bufftable");
				TEST_MODULE_EXPORTS.options_windows("squad");
				TEST_MODULE_EXPORTS.options_windows("dps");
				TEST_MODULE_EXPORTS.options_windows("skills");
				TEST_MODULE_EXPORTS.options_windows("metrics");
				TEST_MODULE_EXPORTS.options_windows("error");
				TEST_MODULE_EXPORTS.options_windows("log");
				TEST_MODULE_EXPORTS.options_windows(nullptr);
			}
			
			ImGui::Checkbox("Logs", &combatMock->showLog);
			if (TEST_MODULE_EXPORTS.options_end != nullptr)
			{
				TEST_MODULE_EXPORTS.options_end();
			}
			
			ImGui::End();
		}

		ImGui::PopStyleVar();

		combatMock->DisplayWindow();

		if (TEST_MODULE_EXPORTS.imgui != nullptr)
		{
			TEST_MODULE_EXPORTS.imgui(static_cast<uint32_t>(true));
		}

		 // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, NULL);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync
	}

	ModReleaseSignature mod_release = reinterpret_cast<ModReleaseSignature>(get_release_addr());
	assert(mod_release != nullptr);
	mod_release();

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
	if (pArgumentCount <= 1)
	{
		assert(false && "Too few arguments sent");
		return 1;
	}

	const char* modulePath = pArgumentVector[1];
	const char* mockFilePath = nullptr;
	if (pArgumentCount >= 3)
	{
		mockFilePath = pArgumentVector[2];
	}

	// create arcdps settings directoy
	if (!(CreateDirectoryA("addons\\", NULL) || ERROR_ALREADY_EXISTS == GetLastError())) {
		assert(false && "Error creating addons directory");
	}
	if (!(CreateDirectoryA("addons\\arcdps\\", NULL) || ERROR_ALREADY_EXISTS == GetLastError())) {
		assert(false && "Error creating arcdps directory");
	}

	return Run(modulePath, mockFilePath);
}

// Helper functions
bool CreateDeviceD3D(HWND hWnd)
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
    if (D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext) != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
	CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = NULL; }
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
		 if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
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