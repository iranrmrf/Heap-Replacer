#pragma once

#include "main/util.h"
#include "main/definitions.h"

#include "memory_pools/memory_pool_manager.h"
#include "default_heap/default_heap_manager.h"
#include "scrap_heap/scrap_heap_manager.h"

#include "graph_data.h"
#include "color_array.h"

#include <d3d9.h>
#include <dinput.h>

#pragma comment(lib, "dxguid.lib")

#define CHECKBOX_COUNT (3)

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class ui
{

private:

	bool enable_input;

	WNDPROC original_wnd_proc;

	float plot_height;

	float refresh_time;
	
	const int slider_lo = 1;
	const int slider_hi = 50;

private: // WINDOWS

	bool enable_ui;
	bool enable_overlay;
	bool enable_background_logging;
	bool enable_info_window;
	bool enable_mpm_window;
	bool enable_dhm_window;
	bool enable_shm_window;

	bool enable_demo_window;
	bool enable_style_editor_window;
	bool enable_user_guide_window;
	bool enable_metrics_window;
	bool enable_about_window;

private: // OVERLAY

	float menu_height;

	bool enable_overlay_fps;
	bool enable_overlay_frametime;
	bool enable_overlay_mouse_pos;
	
	float overlay_color[4];

private: // INFO

	bool enable_info_fps_graph;
	bool enable_info_frametime_graph;

private: // MEMORY POOL MANAGER

	bool enable_mpm_allocs_graph;
	bool enable_mpm_frees_graph;
	bool enable_mpm_total_counters;

	bool enable_mpm_pool_stats;

private: // DEFAULT HEAP MANAGER

	bool enable_dhm_allocs_graph;
	bool enable_dhm_frees_graph;
	bool enable_dhm_free_blocks_graph;

	bool enable_dhm_used;
	bool enable_dhm_free;

	bool enable_dhm_total_counters; // allocs / frees to date

	size_t dhm_block_count;

private: // SCRAP HEAP MANAGER

	bool enable_shm_allocs_graph;
	bool enable_shm_frees_graph;

private:

	const char* names[CHECKBOX_COUNT] = { "FPS Counter", "Frametime Counter", "Mouse XY Counter" };
	bool* bools[CHECKBOX_COUNT] = { &this->enable_overlay_fps, &this->enable_overlay_frametime, &this->enable_overlay_mouse_pos };
	const char* templates[CHECKBOX_COUNT] = { " 0000.0 FPS ", " 000.00 ms ", " (-1000, -1000) " };

#ifdef HR_USE_GUI
	typedef void (get_data)(char*);
	get_data* ops[CHECKBOX_COUNT] =
	{
		[](char* buff) { sprintf(buff, "%.1f FPS", ImGui::GetIO().Framerate); },
		[](char* buff) { sprintf(buff, "%.2f ms", 1000.0f / ImGui::GetIO().Framerate); },
		[](char* buff) { POINT p; GetCursorPos(&p); sprintf(buff, "(%04d, %04d)", p.x, p.y); }
	};
#endif

private:

	graph_data* gd_info_fps;
	graph_data* gd_info_frametime;

	graph_data* gd_mpm_allocs;
	graph_data* gd_mpm_frees;

	graph_data* gd_dhm_allocs;
	graph_data* gd_dhm_frees;
	graph_data* gd_dhm_free_blocks;

	graph_data* gd_shm_allocs;
	graph_data* gd_shm_frees;

public:

	ui();
	~ui();

private:

	void init(HWND wnd, LPDIRECT3DDEVICE9 device);
	void handle_input(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam);

	void set_imgui_mouse_state(bool enabled);

	void init_graph_data_counters();
	void fini_graph_data_counters();

	void render();

	void update_graphs();
	void render_ui();

	void render_game_menu();
	void render_overlay();
	void render_info_window();
	void render_memory_pool_menu();
	void render_default_heap_menu();
	void render_scrap_heap_menu();
	void render_default_heap_blocks();

public:

	static HRESULT(WINAPI* direct_input_8_create)(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, IUnknown* punkOuter);
	static HRESULT(WINAPI* create_devicea)(IDirectInput8A* self, REFGUID rguid, IDirectInputDevice8A** lplpDirectInputDevice, IUnknown* pUnkOuter);
	static HRESULT(APIENTRY* get_device_state)(IDirectInputDevice8A* self, DWORD cbData, LPVOID lpvData);
	static HRESULT(APIENTRY* get_device_data)(IDirectInputDevice8A* self, DWORD cbObjectData, LPDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD dwFlags);
	static HRESULT(APIENTRY* set_cooperative_level)(IDirectInputDevice8A* self, HWND hwnd, DWORD dwFlags);

public:

	static HRESULT WINAPI direct_input_8_create_hook(HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, IUnknown* punkOuter);
	static HRESULT WINAPI create_devicea_hook(IDirectInput8A* self, REFGUID rguid, IDirectInputDevice8A** lplpDirectInputDevice, IUnknown* pUnkOuter);
	static HRESULT APIENTRY get_device_state_hook(IDirectInputDevice8A* self, DWORD cbData, LPVOID lpvData);
	static HRESULT APIENTRY get_device_data_hook(IDirectInputDevice8A* self, DWORD cbObjectData, LPDIDEVICEOBJECTDATA rgdod, LPDWORD pdwInOut, DWORD dwFlags);
	static HRESULT APIENTRY set_cooperative_level_hook(IDirectInputDevice8A* self, HWND hwnd, DWORD dwFlags);

	static LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

public:

	static HWND WINAPI create_window_hook(DWORD dwExStyle, LPCSTR lpClassName, LPCSTR lpWindowName, DWORD dwStyle, int x, int y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu, HINSTANCE hInstance, LPVOID lpParam);
	static LRESULT WINAPI dispatch_message_hook(const MSG* Msg);
	static HRESULT display_scene_hook(IDirect3DDevice9* self);

};
