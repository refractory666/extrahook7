#ifndef MENU_H
#define MENU_H

#include "includes.h"
#include "overlay.h"

struct ImFont;
extern ImFont* g_menu_icon_font;  // icomoon font, 'k' = keyboard icon

class Drawing
{
private:
	static ImVec2 v_window_size;
	static LPCSTR lp_window_name;
	static ImGuiWindowFlags window_flags;
	static bool b_draw;
	static UI::WindowItem lp_selected_window;
	static int m_i_current_tab;
	static bool style_inited;
	static void init_style_once();

	static void tab_visuals();
	static void tab_misc();
	static void tab_config();

public:
	static bool b_obs_bypass;
	static bool is_active();
	static void draw();
	static void* get_memory();
	static size_t get_unity_base();
	static void try_auto_init_esp_once();
};

#endif
