// This file is derived from the example in imgui/examples/example_win32_directx9/main.cpp

#include "CombatMock.h"

#include "../imgui/imgui.h"
#include "../imgui/backends/imgui_impl_dx9.h"
#include "../imgui/backends/imgui_impl_win32.h"

#include <d3d9.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <memory>
#include <tchar.h>

#include "arcdps_structs.h"


extern "C" __declspec(dllexport) void e3(const char* pString);
extern "C" __declspec(dllexport) void e5(ImVec4** colors);
extern "C" __declspec(dllexport) uint64_t e6();
extern "C" __declspec(dllexport) uint64_t e7();
extern "C" __declspec(dllexport) void e8(const char* pString);

// Data
static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
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
static ImVec4 coreCols[11] { // size of enum `n_colours_core`
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
static ImVec4 profColsBase[9] { // size of enum `Prof`
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
static ImVec4 profColsHighlight[9] { // size of enum `Prof`
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

const char* MOCK_VERSION = "ARCDPS_MOCK 0.1";


int Run(const char* pModulePath, const char* pMockFilePath)
{
	// Create application window
	//ImGui_ImplWin32_EnableDpiAwareness();
	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T("Arcdps Mock"), NULL };
	::RegisterClassEx(&wc);
	HWND hwnd = ::CreateWindow(wc.lpszClassName, _T("Arcdps Mock"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

	// Initialize Direct3D
	if (!CreateDeviceD3D(hwnd))
	{
		CleanupDeviceD3D();
		::UnregisterClass(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	// Show the window
	::ShowWindow(hwnd, SW_SHOWDEFAULT);
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
	ImGui_ImplDX9_Init(g_pd3dDevice);

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
	assert(testModuleHandle != NULL);

	auto get_init_addr = reinterpret_cast<GetInitAddrSignature>(GetProcAddress(testModuleHandle, "get_init_addr"));
	assert(get_init_addr != nullptr);
	auto get_release_addr = reinterpret_cast<GetReleaseAddrSignature>(GetProcAddress(testModuleHandle, "get_release_addr"));
	assert(get_release_addr != nullptr);

	ModInitSignature mod_init = reinterpret_cast<ModInitSignature>(get_init_addr(MOCK_VERSION, ImGui::GetCurrentContext(), g_pd3dDevice, selfHandle, malloc, free));
	assert(mod_init != nullptr);
	arcdps_exports* temp_exports = mod_init();
	memcpy(&TEST_MODULE_EXPORTS, temp_exports, sizeof(TEST_MODULE_EXPORTS)); // Maybe do some deep copy at some point but we're not using the strings in there anyways

	combatMock = std::make_unique<CombatMock>(&TEST_MODULE_EXPORTS);
	if (pMockFilePath != nullptr)
	{
		uint32_t result = combatMock->LoadFromFile(pMockFilePath);
		assert(result == 0);
		combatMock->Execute();
	}

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
		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// Show options window (mirror of arcdps options)
		{
			ImGui::Begin("Options");
			ImGui::Checkbox("Logs", &combatMock->showLog);
			if (TEST_MODULE_EXPORTS.options_end != nullptr)
			{
				TEST_MODULE_EXPORTS.options_end();
			}

			if (TEST_MODULE_EXPORTS.options_windows != nullptr) {
				TEST_MODULE_EXPORTS.options_windows("skills");
				TEST_MODULE_EXPORTS.options_windows("metrics");
				TEST_MODULE_EXPORTS.options_windows("dps");
				TEST_MODULE_EXPORTS.options_windows("log");
				TEST_MODULE_EXPORTS.options_windows("bufftable");
				TEST_MODULE_EXPORTS.options_windows("Errors");
				TEST_MODULE_EXPORTS.options_windows(nullptr);
			}
			ImGui::End();
		}

		combatMock->DisplayWindow();

		if (TEST_MODULE_EXPORTS.imgui != nullptr)
		{
			TEST_MODULE_EXPORTS.imgui(static_cast<uint32_t>(true));
		}

		// Rendering
		ImGui::EndFrame();
		g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
		g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
		g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
		D3DCOLOR clear_col_dx = D3DCOLOR_RGBA((int)(clear_color.x * 255.0f), (int)(clear_color.y * 255.0f), (int)(clear_color.z * 255.0f), (int)(clear_color.w * 255.0f));
		g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, clear_col_dx, 1.0f, 0);
		if (g_pd3dDevice->BeginScene() >= 0)
		{
			ImGui::Render();
			ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
			g_pd3dDevice->EndScene();
		}
		HRESULT result = g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

		// Handle loss of D3D9 device
		if (result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
			ResetDevice();
	}

	ModReleaseSignature mod_release = reinterpret_cast<ModReleaseSignature>(get_release_addr());
	assert(mod_release != nullptr);
	mod_release();

	ImGui_ImplDX9_Shutdown();
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

	return Run(modulePath, mockFilePath);
}

// Helper functions
bool CreateDeviceD3D(HWND hWnd)
{
	if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
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
	if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
		return false;

	return true;
}

void CleanupDeviceD3D()
{
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
	if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
}

void ResetDevice()
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
	if (hr == D3DERR_INVALIDCALL)
		IM_ASSERT(0);
	ImGui_ImplDX9_CreateDeviceObjects();
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
			g_d3dpp.BackBufferWidth = LOWORD(lParam);
			g_d3dpp.BackBufferHeight = HIWORD(lParam);
			ResetDevice();
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