#define IMGUI_DEFINE_MATH_OPERATORS
#include "menu.h"
#include "memory.h"
#include "scanner.h"
#include "features/cheat/esp.h"
#include "features/cheat/aimbot.h"
#include "features/cheat/misc.h"
// skinchanger removed in 1.0.0
#include "features/cheat/triggerbot.h"
#include "features/cheat/map_service.h"
#include "settings_menu.h"
#include "imgui_addons.h"
#include "imgui_groups.h"
#include "fonts/IconsFontAwesome6.h"
#include <windows.h>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdio>

ImFont* g_menu_icon_font = nullptr;


LPCSTR Drawing::lp_window_name = "Menu";
ImVec2 Drawing::v_window_size = { 650, 550 };
ImGuiWindowFlags Drawing::window_flags = 0;
bool Drawing::b_draw = true;
bool Drawing::b_obs_bypass = false;
UI::WindowItem Drawing::lp_selected_window = { nullptr, "", "" };
int Drawing::m_i_current_tab = 0;
bool Drawing::style_inited = false;

static mem::Memory g_memory;
static size_t g_unity_base = 0;

static float tab_alpha = 0.f;
static float tab_add = 0.f;
static int active_tab = 0;

const char* cheat_name = "celestial";

static const char* keybind_vk_to_name(int vk) {
    if (vk == 0) return "None";
    switch (vk) {
        case VK_LBUTTON: return "LMB";
        case VK_RBUTTON: return "RMB";
        case VK_MBUTTON: return "M3";
        case VK_XBUTTON1: return "M4";
        case VK_XBUTTON2: return "M5";
        case VK_F1: return "F1"; case VK_F2: return "F2"; case VK_F3: return "F3";
        case VK_F4: return "F4"; case VK_F5: return "F5"; case VK_F6: return "F6";
        case VK_F7: return "F7"; case VK_F8: return "F8"; case VK_F9: return "F9";
        case VK_F10: return "F10"; case VK_F11: return "F11"; case VK_F12: return "F12";
        case VK_SHIFT: return "Shift"; case VK_CONTROL: return "Ctrl"; case VK_MENU: return "Alt";
        case VK_SPACE: return "Space"; case VK_ESCAPE: return "Esc"; case VK_RETURN: return "Enter";
        case VK_TAB: return "Tab"; case VK_CAPITAL: return "Caps";
        default: break;
    }
    static char buf[32];
    UINT sc = MapVirtualKeyW((UINT)vk, MAPVK_VK_TO_VSC);
    if (sc == 0) { snprintf(buf, sizeof(buf), "0x%02X", vk); return buf; }
    LONG lParam = (LONG)(sc << 16);
    if (GetKeyNameTextA(lParam, buf, (int)sizeof(buf)) > 0) return buf;
    snprintf(buf, sizeof(buf), "Key%u", vk);
    return buf;
}

void Drawing::init_style_once()
{
    if (style_inited) return;
    style_inited = true;
    ImGui::StyleColorsDark();
    ImGuiStyle& s = ImGui::GetStyle();

    s.WindowRounding = 3;
    s.ChildRounding = 2;
    s.FrameRounding = 2;
    s.PopupRounding = 2;
    s.GrabRounding = 2;
    s.TabRounding = 2;
    s.ScrollbarRounding = 1;
    s.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    s.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    s.FramePadding = ImVec2(8.0f, 8.0f);
    s.ItemSpacing = ImVec2(14.0f, 14.0f);
    s.WindowPadding = ImVec2(14.0f, 14.0f);
    s.ItemInnerSpacing = ImVec2(8.0f, 4.0f);
    s.WindowBorderSize = 0;
    s.FrameBorderSize = 0;
    s.ScrollbarSize = 12.f;
    s.GrabMinSize = 8.f;

    s.Colors[ImGuiCol_WindowBg] = c::background::filling;
    s.Colors[ImGuiCol_PopupBg] = c::background::filling;
    s.Colors[ImGuiCol_ChildBg] = c::background::child;
    s.Colors[ImGuiCol_MenuBarBg] = c::background::child;
    s.Colors[ImGuiCol_TitleBg] = c::background::child;
    s.Colors[ImGuiCol_ScrollbarBg] = c::background::child;
    s.Colors[ImGuiCol_Border] = c::elements::border;
    s.Colors[ImGuiCol_Separator] = c::elements::seperator;
    s.Colors[ImGuiCol_CheckMark] = c::elements::mark;
    s.Colors[ImGuiCol_Text] = c::elements::text;
    s.Colors[ImGuiCol_TextDisabled] = c::elements::text_disabled;
    s.Colors[ImGuiCol_ScrollbarGrab] = c::elements::scrollbar_grabbed;
    s.Colors[ImGuiCol_ScrollbarGrabHovered] = c::elements::scrollbar_grab_hovered;
    s.Colors[ImGuiCol_ScrollbarGrabActive] = c::elements::scrollbar_active;
    s.Colors[ImGuiCol_SliderGrab] = c::accent;
    s.Colors[ImGuiCol_SliderGrabActive] = c::accent;
    s.Colors[ImGuiCol_Button] = c::elements::button;
    s.Colors[ImGuiCol_ButtonHovered] = c::elements::button_hovered;
    s.Colors[ImGuiCol_ButtonActive] = c::elements::button_active;
    s.Colors[ImGuiCol_FrameBg] = c::elements::button;
    s.Colors[ImGuiCol_FrameBgHovered] = c::elements::button_hovered;
    s.Colors[ImGuiCol_FrameBgActive] = c::elements::button_active;
    s.Colors[ImGuiCol_Header] = c::background::child;
    s.Colors[ImGuiCol_HeaderHovered] = ImVec4(c::background::child.x, c::background::child.y, c::background::child.z, 0.7f);
    s.Colors[ImGuiCol_HeaderActive] = ImVec4(c::background::child.x, c::background::child.y, c::background::child.z, 0.5f);
}

void Drawing::tab_visuals()
{
    ImGui::Checkbox("ESP", &esp::g_esp_enabled);
    ImGui::Checkbox("Boxes", &esp::g_esp_box);
    ImGui::SameLine();
    ImGui::ColorEdit4("##box", esp::g_esp_color_box, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    ImGui::Checkbox("HP", &esp::g_esp_hp);
    ImGui::SameLine();
    ImGui::ColorEdit4("##hp_full", esp::g_esp_color_hp_full, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    ImGui::SameLine();
    ImGui::ColorEdit4("##hp_low", esp::g_esp_color_hp_low, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    ImGui::Checkbox("Nicknames", &esp::g_esp_name);
    ImGui::SameLine();
    ImGui::ColorEdit4("##name", esp::g_esp_color_name, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    ImGui::Checkbox("Avatars", &esp::g_esp_avatar);
    ImGui::Checkbox("Skeleton", &esp::g_esp_skeleton);
    ImGui::SameLine();
    ImGui::ColorEdit4("##skeleton", esp::g_esp_color_skeleton, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    ImGui::Checkbox("Weapon", &esp::g_esp_weapon);
    ImGui::Checkbox("Armor", &esp::g_esp_armor);
    ImGui::SameLine();
    ImGui::ColorEdit4("##armor", esp::g_esp_color_armor, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    ImGui::SameLine();
    ImGui::ColorEdit4("##armor_low", esp::g_esp_color_armor_low, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
}

void Drawing::tab_misc()
{
    ImGui::Checkbox("Hide from record", &b_obs_bypass);
    ImGui::Separator();
    ImGui::Checkbox("Aimbot", &esp::g_aimbot_enabled);
    ImGui::SliderFloat("FOV", &esp::g_aimbot_fov, 1.f, 180.f, "%.0f");
    ImGui::Checkbox("Show FOV circle", &esp::g_aimbot_fov_display);
    const char* mode_items[] = { "Hold", "Toggle" };
    ImGui::Combo("Mode", &esp::g_aimbot_mode, mode_items, 2);
    const char* target_items[] = { "Head", "Body", "Legs" };
    ImGui::Combo("Target", &esp::g_aimbot_target, target_items, 3);
}

void Drawing::tab_config()
{
}

void* Drawing::get_memory() { return &g_memory; }
size_t Drawing::get_unity_base() { return g_unity_base; }

void Drawing::try_auto_init_esp_once()
{
    static bool s_done = false;
    if (s_done) return;
    s_done = true;
    if (!g_memory.Init()) return;
    if (!g_memory.WaitCapture(3000)) return;
    Scanner::SetMemoryInstance(&g_memory);
    size_t base = Scanner::FindLibUnity();
    if (!base) return;
    g_unity_base = base;
    esp::read_client_world(&g_memory, g_unity_base);
}

bool Drawing::is_active() { return b_draw; }

void Drawing::draw()
{
    if ((GetAsyncKeyState(VK_DELETE) & 1) || (GetAsyncKeyState(VK_INSERT) & 1))
        b_draw = !b_draw;

    if (!b_draw)
        return;

    init_style_once();
    ImGuiStyle& style = ImGui::GetStyle();

    static float ui_dpi_scale = 0.f;
    ui_dpi_scale = ImLerp(ui_dpi_scale, 1.0f, 0.5f * (12.0f * ImGui::GetIO().DeltaTime));

    ImGui::SetNextWindowSize(ImVec2(c::size.x, c::size.y * ui_dpi_scale));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::Begin("menu", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoResize);
    ImGui::PopStyleVar(2);

    style.Colors[ImGuiCol_SliderGrab] = c::accent;
    style.Colors[ImGuiCol_SliderGrabActive] = c::accent;

    ImVec2 pos = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    float new_x = ImLerp(c::size.x, 650.0f, ImGui::GetIO().DeltaTime * 15.0f);
    c::size = ImVec2(new_x, c::size.y);

    const float fh = ImGui::GetFrameHeight();
    drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + fh), ImGui::GetColorU32(ImGuiCol_ChildBg), style.WindowRounding, ImDrawFlags_RoundCornersTop);
    drawList->AddRectFilled(ImVec2(pos.x, pos.y + fh), ImVec2(pos.x + size.x, pos.y + size.y - fh), ImGui::GetColorU32(ImGuiCol_WindowBg), style.WindowRounding, ImDrawFlags_RoundCornersNone);
    drawList->AddRectFilled(ImVec2(pos.x, pos.y + size.y - fh), ImVec2(pos.x + size.x, pos.y + size.y), ImGui::GetColorU32(ImGuiCol_ChildBg), style.WindowRounding, ImDrawFlags_RoundCornersBottom);
    const char* title_main = "quadrohud";
    const char* title_accent = ".cc";
    const ImVec2 sz_main = ImGui::CalcTextSize(title_main);
    const ImVec2 sz_accent = ImGui::CalcTextSize(title_accent);
    const float total_w = sz_main.x + sz_accent.x;
    const float start_x = pos.x + (size.x - total_w) * 0.5f;
    const float text_y = pos.y + (fh - sz_main.y) * 0.5f;
    drawList->AddText(ImVec2(start_x, text_y), IM_COL32(255, 255, 255, 255), title_main);
    drawList->AddText(ImVec2(start_x + sz_main.x, text_y), ImGui::GetColorU32(ImGuiCol_SliderGrab), title_accent);

    static int CurrentPage = 0;
    static float SideBarWidth = 150;

    ImGui::SetCursorPosY(ImGui::GetFrameHeight() + 10);
    ImGui::BeginGroup();

    ImGui::BeginChild("Sidebar", ImVec2(SideBarWidth, ImGui::GetWindowHeight() - ImGui::GetFrameHeight() * 2), true, ImGuiWindowFlags_NoBackground);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, style.Colors[ImGuiCol_ChildBg]);
    ImAdd::RadioButtonIcon("aimbot", ICON_FA_CROSSHAIRS, "Aimbot", &CurrentPage, 0, ImVec2(-0.1f, 0));
    ImAdd::RadioButtonIcon("players", ICON_FA_USER, "Players", &CurrentPage, 1, ImVec2(-0.1f, 0));
    ImAdd::RadioButtonIcon("misc", ICON_FA_PLAY, "Misc", &CurrentPage, 2, ImVec2(-0.1f, 0));
    ImAdd::RadioButtonIcon("guns", ICON_FA_GUN, "Guns", &CurrentPage, 4, ImVec2(-0.1f, 0));
    ImGui::Separator();
    ImAdd::RadioButtonIcon("configs", ICON_FA_LIST, "Configs", &CurrentPage, 3, ImVec2(-0.1f, 0));
    ImGui::PopStyleColor();
    ImGui::EndChild();

    tab_alpha = ImLerp(tab_alpha, (CurrentPage == active_tab) ? 1.f : 0.f, 12.f * ImGui::GetIO().DeltaTime);
    if (tab_alpha < 0.01f && tab_add < 0.01f) active_tab = CurrentPage;

    ImGui::SetCursorPos(ImVec2(SideBarWidth - style.WindowPadding.x, 140 - (tab_alpha * 100)));
    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, tab_alpha * style.Alpha);

    if (active_tab == 0)  // Aimbot
    {
        ImGui::BeginChild("Content", ImVec2(0, ImGui::GetWindowHeight() - ImGui::GetFrameHeight() * 2), true, ImGuiWindowFlags_NoBackground);
        ImGrp::Checkbox("Aimbot", &esp::g_aimbot_enabled);
        if (ImAdd::Keybind("Keybind", &esp::g_aimbot_key)) {}
        ImAdd::SliderFloat("FOV", &esp::g_aimbot_fov, 1.f, 180.f, "%.0f");
        const char* mode_items[] = { "Hold", "Toggle" };
        static int mode = esp::g_aimbot_mode;
        mode = esp::g_aimbot_mode;
        if (ImAdd::Combo("Mode", "Select mode", &mode, mode_items, 2, 2))
            esp::g_aimbot_mode = mode;
        const char* target_items[] = { "Head", "Body", "Legs", "Multipoints" };
        static int target = esp::g_aimbot_target;
        target = esp::g_aimbot_target;
        if (ImAdd::Combo("Target", "Select target", &target, target_items, 4, 4))
            esp::g_aimbot_target = target;
        ImGrp::Checkbox("Visible check", &esp::g_aimbot_visible_check);
        ImAdd::SliderFloat("Return aim (ms)", &esp::g_aimbot_return_ms, 0.f, 100.f, "%.0f");
        ImGui::Separator();
        ImGrp::Checkbox("Anti-aim", &esp::g_antiaim_enabled);
        ImAdd::SliderFloat("Anti-aim pitch", &esp::g_antiaim_pitch_amp, 0.f, 10.f, "%.1f");
        ImAdd::SliderFloat("Anti-aim yaw", &esp::g_antiaim_yaw_amp, 0.f, 10.f, "%.1f");
        ImGrp::Checkbox("Show radius", &esp::g_aimbot_fov_display);
        ImGui::EndChild();
    }

    if (active_tab == 1)  // Players
    {
        ImGui::BeginChild("Content", ImVec2(0, ImGui::GetWindowHeight() - ImGui::GetFrameHeight() * 2), true, ImGuiWindowFlags_NoBackground);
        const ImGuiStyle& st = ImGui::GetStyle();
        const float fh = ImGui::GetFrameHeight();
        const float sq = ImGui::GetFontSize();

        auto row_color_check = [&](const char* label, bool* v, float col[4]) {
            ImGui::PushID(label);
            ImGui::BeginChild("row", ImVec2(0, fh + st.FramePadding.y * 2), false);
            ImGui::SetCursorPos(st.FramePadding * 2);
            ImGui::TextUnformatted(label);
            ImGui::SameLine(0, st.ItemInnerSpacing.x);
            ImGui::SetCursorPosY(st.FramePadding.y * 2);
            ImVec4 c(col[0], col[1], col[2], col[3]);
            if (ImGui::ColorButton("##col", c, ImGuiColorEditFlags_NoTooltip, ImVec2(sq, sq)))
                ImGui::OpenPopup("##popup");
            if (ImGui::BeginPopup("##popup")) {
                ImGui::ColorPicker4("##pick", col, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
                ImGui::EndPopup();
            }
            ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetFontSize() - st.FramePadding.x * 2.5f);
            ImGui::SetCursorPosY(st.FramePadding.y * 2);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            ImAdd::SmallCheckbox("##c", v);
            ImGui::PopStyleVar();
            ImGui::EndChild();
            ImGui::PopID();
        };

        auto row_gradient_check = [&](const char* label, bool* v, float col1[4], float col2[4]) {
            ImGui::PushID(label);
            ImGui::BeginChild("row", ImVec2(0, fh + st.FramePadding.y * 2), false);
            ImGui::SetCursorPos(st.FramePadding * 2);
            ImGui::TextUnformatted(label);
            ImGui::SameLine(0, st.ItemInnerSpacing.x);
            ImGui::SetCursorPosY(st.FramePadding.y * 2);
            if (ImGui::ColorButton("##col1", ImVec4(col1[0], col1[1], col1[2], col1[3]), ImGuiColorEditFlags_NoTooltip, ImVec2(sq, sq)))
                ImGui::OpenPopup("##popup1");
            if (ImGui::BeginPopup("##popup1")) {
                ImGui::ColorPicker4("##pick1", col1, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
                ImGui::EndPopup();
            }
            ImGui::SameLine(0, st.ItemInnerSpacing.x);
            if (ImGui::ColorButton("##col2", ImVec4(col2[0], col2[1], col2[2], col2[3]), ImGuiColorEditFlags_NoTooltip, ImVec2(sq, sq)))
                ImGui::OpenPopup("##popup2");
            if (ImGui::BeginPopup("##popup2")) {
                ImGui::ColorPicker4("##pick2", col2, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
                ImGui::EndPopup();
            }
            ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetFontSize() - st.FramePadding.x * 2.5f);
            ImGui::SetCursorPosY(st.FramePadding.y * 2);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            ImAdd::SmallCheckbox("##c", v);
            ImGui::PopStyleVar();
            ImGui::EndChild();
            ImGui::PopID();
        };

        row_color_check("Boxes", &esp::g_esp_box, esp::g_esp_color_box);
        row_gradient_check("HP", &esp::g_esp_hp, esp::g_esp_color_hp_full, esp::g_esp_color_hp_low);
        row_gradient_check("Armor", &esp::g_esp_armor, esp::g_esp_color_armor, esp::g_esp_color_armor_low);
        row_color_check("Nicknames", &esp::g_esp_name, esp::g_esp_color_name);
        ImGrp::Checkbox("Avatars", &esp::g_esp_avatar);
        row_color_check("Skeleton", &esp::g_esp_skeleton, esp::g_esp_color_skeleton);
        row_color_check("Weapon", &esp::g_esp_weapon, esp::g_esp_color_weapon);
        row_color_check("Chams", &esp::g_esp_hitbox_outline_fill, esp::g_esp_color_hitbox_fill);
        row_color_check("Glow", &esp::g_esp_hitbox_glow, esp::g_esp_glow_color);
        ImGui::EndChild();
    }

    if (active_tab == 2)  // Misc
    {
        ImGui::BeginChild("Content", ImVec2(0, ImGui::GetWindowHeight() - ImGui::GetFrameHeight() * 2), true, ImGuiWindowFlags_NoBackground);
        ImGrp::Checkbox("Stop", &misc::g_misc_stop_enabled);
        if (ImAdd::Keybind("Stop keybind", &misc::g_misc_stop_key)) {}
        ImGrp::Checkbox("Teleport", &misc::g_misc_teleport_enabled);
        if (ImAdd::Keybind("Teleport keybind", &misc::g_misc_teleport_key)) {}
        ImGrp::Checkbox("Show material", &misc::g_misc_show_material);
        ImGrp::Checkbox("Hide from record", &b_obs_bypass);

        // Ручная установка HP локального игрока через Photon-проперти "health".
        static char health_input[16] = "";
        ImGui::Separator();
        ImGui::TextUnformatted("Set local HP");
        ImGui::SetNextItemWidth(100.0f);
        ImGui::InputTextWithHint("##health_input", "52", health_input, sizeof(health_input), ImGuiInputTextFlags_CharsDecimal);
        ImGui::SameLine();
        if (ImGui::Button("Apply"))
        {
            // Простой безопасный парсинг числа без использования sscanf.
            int hp = 0;
            if (health_input[0] != '\0')
            {
                hp = std::atoi(health_input);
                misc::set_local_health(&g_memory, hp);
            }
        }

        // Ручная установка целочисленной Photon-проперти всем игрокам.
        static char badge_prop[32] = "badge";
        static char badge_input[16] = "";
        ImGui::TextUnformatted("Set int prop for all");
        ImGui::SetNextItemWidth(120.0f);
        ImGui::InputTextWithHint("##badge_prop", "prop name (e.g. badgeId)", badge_prop, sizeof(badge_prop));
        ImGui::SetNextItemWidth(80.0f);
        ImGui::InputTextWithHint("##badge_input", "value", badge_input, sizeof(badge_input), ImGuiInputTextFlags_CharsDecimal);
        ImGui::SameLine();
        if (ImGui::Button("Apply prop"))
        {
            if (badge_input[0] != '\0' && badge_prop[0] != '\0')
            {
                int badge = std::atoi(badge_input);
                misc::set_local_badge(&g_memory, badge_prop, badge);
            }
        }

        ImGui::Separator();
        ImGrp::Checkbox("Enable World FOV", &misc::g_fov_enabled);
        if (misc::g_fov_enabled)
            ImAdd::SliderFloat("FOV Value", &misc::g_fov_value, 30.f, 150.f, "%.0f");

        ImGui::Separator();
        ImGrp::Checkbox("Thirdperson (Experimental)", &misc::g_thirdperson_enabled);
        if (misc::g_thirdperson_enabled)
            ImAdd::SliderFloat("Camera Distance", &misc::g_thirdperson_dist, 0.f, 5.f, "%.1f");

        ImGui::Separator();
        ImGrp::Checkbox("Force Crouch", &misc::g_crouch_enabled);
        ImGrp::Checkbox("Force Aim/Scope", &misc::g_aim_enabled);
        ImGrp::Checkbox("Force Jump", &misc::g_jump_enabled);
        ImGrp::Checkbox("Force Shoot", &misc::g_shoot_enabled);

        const ImGuiStyle& stM = ImGui::GetStyle();
        const float fhM = ImGui::GetFrameHeight();
        const float sqM = ImGui::GetFontSize();
        ImGui::PushID("Accent color");
        ImGui::BeginChild("row_accent", ImVec2(0, fhM + stM.FramePadding.y * 2), false);
        ImGui::SetCursorPos(stM.FramePadding * 2);
        ImGui::TextUnformatted("Accent color");
        ImGui::SameLine(0, stM.ItemInnerSpacing.x);
        ImGui::SetCursorPosY(stM.FramePadding.y * 2);
        if (ImGui::ColorButton("##accent_col", c::accent, ImGuiColorEditFlags_NoTooltip, ImVec2(sqM, sqM)))
            ImGui::OpenPopup("##accent_popup");
        if (ImGui::BeginPopup("##accent_popup")) {
            ImGui::ColorPicker4("##accent_pick", &c::accent.x, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
            ImGui::EndPopup();
        }
        ImGui::EndChild();
        ImGui::PopID();
        ImGui::EndChild();
    }

    if (active_tab == 4)  // Guns
    {
        ImGui::BeginChild("Content", ImVec2(0, ImGui::GetWindowHeight() - ImGui::GetFrameHeight() * 2), true, ImGuiWindowFlags_NoBackground);
        ImGrp::Checkbox("Triggerbot", &triggerbot::g_trigger_enabled);
        if (ImAdd::Keybind("Triggerbot keybind", &triggerbot::g_trigger_key_vk)) {}
        const char* trigger_mode_items[] = { "Hold", "Toggle" };
        static int trigger_mode = triggerbot::g_trigger_mode;
        trigger_mode = triggerbot::g_trigger_mode;
        if (ImAdd::Combo("Mode", "Select mode", &trigger_mode, trigger_mode_items, 2, 2))
            triggerbot::g_trigger_mode = trigger_mode;
        ImGui::Separator();
        ImGrp::Checkbox("Show colliders", &map_service::g_enabled);
        ImGui::Text("Map");
        if (ImGui::Combo("##collider_map", &map_service::g_collider_map_index, map_service::g_collider_map_display_names, map_service::g_collider_map_count)) {
            map_service::LoadColliderMapByIndex(map_service::g_collider_map_index);
        }
        ImGui::EndChild();
    }

    if (active_tab == 3)  // Configs
    {
        ImGui::BeginChild("Content", ImVec2(0, ImGui::GetWindowHeight() - ImGui::GetFrameHeight() * 2), true, ImGuiWindowFlags_NoBackground);
        ImGui::EndChild();
    }

    ImGui::PopStyleVar();
    ImGui::EndGroup();
    ImGui::End();
}
