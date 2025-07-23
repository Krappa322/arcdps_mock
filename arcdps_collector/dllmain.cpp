#include "Collector.h"
#include "GUI.h"
#include "Log.h"

#include <atomic>
#include <mutex>

#include <assert.h>
#include <d3d9helper.h>
#include <stdint.h>
#include <stdio.h>
#include <Windows.h>
#include <imgui/imgui.h>
#include <ArcdpsExtension/arcdps_structs.h>

/* proto/globals */
void dll_init(HANDLE pModule);
void dll_exit();
arcdps_exports* mod_init();
uintptr_t mod_release();
void mod_imgui(uint32_t pNotCharselOrLoading, uint32_t hide_if_combat_or_ooc);
void mod_options_end();
void mod_combat(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t pId, uint64_t pRevision);
void mod_combat_local(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t pId, uint64_t pRevision);

static MallocSignature ARCDPS_MALLOC = nullptr;
static FreeSignature ARCDPS_FREE = nullptr;
static arcdps_exports ARC_EXPORTS;
static const char* ARCDPS_VERSION;

// To be as accurate as possible, first thing we do when receiving an event is to allocate a sequence number. We pass
// that sequence number as an argument when storing the event. This is pretty much as fair as we can possibly be with
// regards to reproducing race conditions. Allocating seq under lock in Collector or something like that would be less
// fair because mutexes are not guaranteed to release in the same order that they were locked
static std::atomic_uint32_t NEXT_EVENT_SEQ = 0;
static Collector COLLECTOR;
static GUI GUI_CONTEXT{COLLECTOR};

/* dll main -- winapi */
BOOL APIENTRY DllMain(HANDLE pModule, DWORD pReasonForCall, LPVOID pReserved)
{
	switch (pReasonForCall) {
	case DLL_PROCESS_ATTACH: dll_init(pModule); break;
	case DLL_PROCESS_DETACH: dll_exit(); break;

	case DLL_THREAD_ATTACH:  break;
	case DLL_THREAD_DETACH:  break;
	}
	return 1;
}

/* dll attach -- from winapi */
void dll_init(HANDLE pModule)
{
	return;
}

/* dll detach -- from winapi */
void dll_exit()
{
	return;
}

static void* MallocWrapper(size_t pSize, void* pUserData)
{
	return ARCDPS_MALLOC(pSize);
}

static void FreeWrapper(void* pPointer, void* pUserData)
{
	ARCDPS_FREE(pPointer);
}

/* export -- arcdps looks for this exported function and calls the address it returns on client load */
extern "C" __declspec(dllexport) void* get_init_addr(const char* pArcdpsVersionString, void* pImguiContext, IDirect3DDevice9 * pUnused, HMODULE pArcModule, MallocSignature pArcdpsMalloc, FreeSignature pArcdpsFree)
{
	ARCDPS_VERSION = pArcdpsVersionString;

	ARCDPS_MALLOC = pArcdpsMalloc;
	ARCDPS_FREE = pArcdpsFree;
	ImGui::SetAllocatorFunctions(MallocWrapper, FreeWrapper);
	ImGui::SetCurrentContext(static_cast<ImGuiContext*>(pImguiContext));

	return mod_init;
}

/* export -- arcdps looks for this exported function and calls the address it returns on client exit */
extern "C" __declspec(dllexport) void* get_release_addr()
{
	ARCDPS_VERSION = nullptr;
	return mod_release;
}

/* initialize mod -- return table that arcdps will use for callbacks */
arcdps_exports* mod_init()
{
#ifdef DEBUG
	AllocConsole();
	SetConsoleOutputCP(CP_UTF8);

	/* big buffer */
	char buff[4096];
	char* p = &buff[0];
	p += _snprintf(p, 400, "==== mod_init ====\n");
	p += _snprintf(p, 400, "arcdps: %s\n", ARCDPS_VERSION);

	/* print */
	DWORD written = 0;
	HANDLE hnd = GetStdHandle(STD_OUTPUT_HANDLE);
	WriteConsoleA(hnd, &buff[0], (DWORD)(p - &buff[0]), &written, 0);
#endif // DEBUG

	memset(&ARC_EXPORTS, 0, sizeof(arcdps_exports));
	ARC_EXPORTS.sig = 0x516a172a;
	ARC_EXPORTS.imguivers = IMGUI_VERSION_NUM;
	ARC_EXPORTS.size = sizeof(arcdps_exports);
	ARC_EXPORTS.out_name = "event_collector";
	ARC_EXPORTS.out_build = "1.0";
	ARC_EXPORTS.combat = mod_combat;
	ARC_EXPORTS.imgui = mod_imgui;
	ARC_EXPORTS.options_end = mod_options_end;
	ARC_EXPORTS.combat_local = mod_combat_local;
	return &ARC_EXPORTS;
}

/* release mod -- return ignored */
uintptr_t mod_release()
{
#ifdef DEBUG
	FreeConsole();
#endif

	return 0;
}

void mod_imgui(uint32_t pNotCharacterSelectionOrLoading, uint32_t hide_if_combat_or_ooc)
{
	if (pNotCharacterSelectionOrLoading == 0)
	{
		return;
	}

	GUI_CONTEXT.Display();
}

void mod_options_end()
{
	GUI_CONTEXT.DisplayOptions();
}

void mod_combat(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t pId, uint64_t pRevision)
{
	uint32_t seq = NEXT_EVENT_SEQ.fetch_add(1, std::memory_order_relaxed);
	COLLECTOR.Add(
		XevtcEventSource::Area,
		seq,
		pEvent,
		pSourceAgent,
		pDestinationAgent,
		pSkillname,
		pId,
		pRevision);
}

void mod_combat_local(cbtevent* pEvent, ag* pSourceAgent, ag* pDestinationAgent, const char* pSkillname, uint64_t pId, uint64_t pRevision)
{
	uint32_t seq = NEXT_EVENT_SEQ.fetch_add(1, std::memory_order_relaxed);
	COLLECTOR.Add(
		XevtcEventSource::Local,
		seq,
		pEvent,
		pSourceAgent,
		pDestinationAgent,
		pSkillname,
		pId,
		pRevision);
}
