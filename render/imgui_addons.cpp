#define IMGUI_DEFINE_MATH_OPERATORS
#ifndef IM_TRUNC
#define IM_TRUNC(_VAL) ((float)(int)(_VAL))
#endif
#include "imgui_addons.h"
#include "settings_menu.h"
#include "fonts/IconsFontAwesome6.h"
#include <map>
#include <string>
#include <unordered_map>

using namespace ImGui;

namespace {
static void DrawShadowRectFallback(ImDrawList* dl, const ImVec2& min, const ImVec2& max, ImU32 col, float thickness, float rounding) {
    if ((col & IM_COL32_A_MASK) == 0) return;
    ImU32 soft = (col & 0x00FFFFFF) | ((ImU32)(0.12f * 255) << 24);
    dl->AddRectFilled(ImVec2(min.x - thickness, min.y - thickness), ImVec2(max.x + thickness, max.y + thickness), soft, rounding + thickness * 0.5f);
}
static void DrawShadowCircleFallback(ImDrawList* dl, const ImVec2& center, float radius, ImU32 col, float thickness) {
    if ((col & IM_COL32_A_MASK) == 0) return;
    ImU32 soft = (col & 0x00FFFFFF) | ((ImU32)(0.12f * 255) << 24);
    dl->AddCircleFilled(center, radius + thickness, soft, 24);
}
}

struct DefaultStyles_State {
    ImVec4 Text;
    ImVec4 TextDisabled;
    ImVec4 WindowBg;
    ImVec4 ChildBg;
    ImVec4 PopupBg;
    ImVec4 Border;
    ImVec4 BorderShadow;
    ImVec4 FrameBg;
    ImVec4 FrameBgHovered;
    ImVec4 FrameBgActive;
    ImVec4 TitleBg;
    ImVec4 TitleBgActive;
    ImVec4 TitleBgCollapsed;
    ImVec4 MenuBarBg;
    ImVec4 ScrollbarBg;
    ImVec4 ScrollbarGrab;
    ImVec4 ScrollbarGrabHovered;
    ImVec4 ScrollbarGrabActive;
    ImVec4 CheckMark;
    ImVec4 SliderGrab;
    ImVec4 SliderGrabActive;
    ImVec4 Button;
    ImVec4 ButtonHovered;
    ImVec4 ButtonActive;
    ImVec4 Header;
    ImVec4 HeaderHovered;
    ImVec4 HeaderActive;
    ImVec4 Separator;
    ImVec4 SeparatorHovered;
    ImVec4 SeparatorActive;
    ImVec4 ResizeGrip;
    ImVec4 ResizeGripHovered;
    ImVec4 ResizeGripActive;
    ImVec4 Tab;
    ImVec4 TabHovered;
    ImVec4 TabActive;
    ImVec4 TabUnfocused;
    ImVec4 TabUnfocusedActive;
    ImVec4 PlotLines;
    ImVec4 PlotLinesHovered;
    ImVec4 PlotHistogram;
    ImVec4 PlotHistogramHovered;
    ImVec4 TableHeaderBg;
    ImVec4 TableBorderStrong;
    ImVec4 TableBorderLight;
    ImVec4 TableRowBg;
    ImVec4 TableRowBgAlt;
    ImVec4 TextSelectedBg;
    ImVec4 DragDropTarget;
    ImVec4 NavHighlight;
    ImVec4 NavWindowingHighlight;
    ImVec4 NavWindowingDimBg;
    ImVec4 ModalWindowDimBg;
    ImVec4 WindowShadow;
};

ImVec4 ImAdd::HexToColorVec4(unsigned int hex_color, float alpha) {
    ImVec4 color;

    color.x = ((hex_color >> 16) & 0xFF) / 255.0f;
    color.y = ((hex_color >> 8) & 0xFF) / 255.0f;
    color.z = (hex_color & 0xFF) / 255.0f;
    color.w = alpha;

    return color;
}

const char* keys[] =
{
    "-",
    "M1",
    "M2",
    "CN",
    "M3",
    "M4",
    "M5",
    "-",
    "BACK",
    "TAB",
    "-",
    "-",
    "CLR",
    "ENTER",
    "-",
    "-",
    "SHIFT",
    "CTRL",
    "Menu",
    "Pause",
    "CAPS",
    "KAN",
    "-",
    "JUN",
    "FIN",
    "KAN",
    "-",
    "ESC",
    "CON",
    "NCO",
    "ACC",
    "MAD",
    "SPACE",
    "PGU",
    "PGD",
    "END",
    "HOME",
    "LEFT",
    "UP",
    "RIGHT",
    "DOWN",
    "SEL",
    "PRI",
    "EXE",
    "PRI",
    "INS",
    "DEL",
    "HEL",
    "0",
    "1",
    "2",
    "3",
    "4",
    "5",
    "6",
    "7",
    "8",
    "9",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "A",
    "B",
    "C",
    "D",
    "E",
    "F",
    "G",
    "H",
    "I",
    "J",
    "K",
    "L",
    "M",
    "N",
    "O",
    "P",
    "Q",
    "R",
    "S",
    "T",
    "U",
    "V",
    "W",
    "X",
    "Y",
    "Z",
    "WIN",
    "WIN",
    "APP",
    "-",
    "SLE",
    "NUM0",
    "NUM1",
    "NUM2",
    "NUM3",
    "NUM4",
    "NUM5",
    "NUM6",
    "NUM7",
    "NUM8",
    "NUM9",
    "MUL",
    "ADD",
    "SEP",
    "MIN",
    "DEL",
    "DIV",
    "F1",
    "F2",
    "F3",
    "F4",
    "F5",
    "F6",
    "F7",
    "F8",
    "F9",
    "F10",
    "F11",
    "F12",
    "F13",
    "F14",
    "F15",
    "F16",
    "F17",
    "F18",
    "F19",
    "F20",
    "F21",
    "F22",
    "F23",
    "F24",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "NUM",
    "SCR",
    "EQU",
    "MAS",
    "TOY",
    "OYA",
    "OYA",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "-",
    "SHIFT",
    "SHIFT",
    "CTRL",
    "CTRL",
    "ALT",
    "ALT"
};

struct key_state
{
    ImVec4 background, text;
    bool active = false;
    bool hovered = false;
    float alpha = 0.f;
};

void RenderTextColor(const ImVec2& p_min, const ImVec2& p_max, ImU32 col, const char* text, const ImVec2& align)
{
    PushStyleColor(ImGuiCol_Text, col);
    RenderTextClipped(p_min, p_max, text, NULL, NULL, align, NULL);
    PopStyleColor();
}

bool ImAdd::Keybind(const char* label, int* key)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    ImGuiIO& io = g.IO;
    const ImGuiStyle& style = g.Style;

    const ImGuiID id = window->GetID(label);
    const float width = (GetContentRegionMax().x - style.WindowPadding.x);

    const ImRect rect(window->DC.CursorPos, window->DC.CursorPos + ImVec2(width, 50));
    const ImRect clickable(window->DC.CursorPos + ImVec2(width - 80, 10), window->DC.CursorPos + ImVec2(width - 10, 40));

    ItemSize(ImRect(rect.Min, rect.Max));
    if (!ImGui::ItemAdd(rect, id)) return false;

    char buf_display[64] = "None";


    bool value_changed = false;
    int k = *key;

    std::string active_key = "";
    active_key += keys[*key];

    if (*key != 0 && g.ActiveId != id) {
        strcpy_s(buf_display, active_key.c_str());
    }
    else if (g.ActiveId == id) {
        strcpy_s(buf_display, "...");
    }

    const ImVec2 label_size = CalcTextSize(buf_display, NULL, true);

    bool hovered = ItemHoverable(rect, id);

    static std::map<ImGuiID, key_state> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end())
    {
        anim.insert({ id, key_state() });
        it_anim = anim.find(id);
    }


    it_anim->second.text = ImLerp(it_anim->second.text, g.ActiveId == id ? c::elements::text_active : hovered ? c::elements::text_hov : c::elements::text, ImGui::GetIO().DeltaTime * 6.f);

    window->DrawList->AddRectFilled(rect.Min, rect.Max, GetColorU32(ImGuiCol_ChildBg), style.FrameRounding);

    window->DrawList->AddRectFilled(clickable.Min, clickable.Max, GetColorU32(ImGuiCol_WindowBg), style.FrameRounding);

    RenderTextColor(rect.Min + ImVec2(15, 10), rect.Max, GetColorU32(ImGuiCol_Text), label, ImVec2(0.0, 0.2));


    RenderTextColor(clickable.Min, clickable.Max, GetColorU32(ImGuiCol_Text), buf_display, ImVec2(0.5f, 0.5f));

    if (hovered && io.MouseClicked[0])
    {
        if (g.ActiveId != id) {

            memset(io.MouseDown, 0, sizeof(io.MouseDown));
            memset(io.KeysDown, 0, sizeof(io.KeysDown));
            *key = 0;
        }
        ImGui::SetActiveID(id, window);
        ImGui::FocusWindow(window);
    }
    else if (io.MouseClicked[0]) {

        if (g.ActiveId == id)
            ImGui::ClearActiveID();
    }

    if (g.ActiveId == id) {
        for (auto i = 0; i < 5; i++) {
            if (io.MouseDown[i]) {
                switch (i) {
                case 0:
                    k = 0x01;
                    break;
                case 1:
                    k = 0x02;
                    break;
                case 2:
                    k = 0x04;
                    break;
                case 3:
                    k = 0x05;
                    break;
                case 4:
                    k = 0x06;
                    break;
                }
                value_changed = true;
                ImGui::ClearActiveID();
            }
        }
        if (!value_changed) {
            for (auto i = 0x08; i <= 0xA5; i++) {
                if (io.KeysDown[i]) {
                    k = i;
                    value_changed = true;
                    ImGui::ClearActiveID();
                }
            }
        }

        if (IsKeyPressedMap(ImGuiKey_Escape)) {
            *key = 0;
            ImGui::ClearActiveID();
        }
        else {
            *key = k;
        }
    }

    return value_changed;
}

bool ImAdd::TextButton(const char* label, const ImVec2& size_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(size_arg, label_size.x, label_size.y);

    const ImRect bb(pos, pos + size);
    ItemSize(size);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);

    // Animate Colors
    static std::map<ImGuiID, DefaultStyles_State> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end())
    {
        anim.insert({ id, DefaultStyles_State() });
        it_anim = anim.find(id);
    }

    it_anim->second.Text = ImLerp(it_anim->second.Text, GetStyleColorVec4((held && hovered) ? ImGuiCol_SliderGrab : hovered ? ImGuiCol_Text : ImGuiCol_TextDisabled), 1.0f / fAnimationSpeed * ImGui::GetIO().DeltaTime);
    ImU32 textCol = GetColorU32(it_anim->second.Text);

    // Render
    RenderNavHighlight(bb, id);

    //window->DrawList->AddText(pos + ImVec2(size.x / 2 - label_size.x / 2, size.y / 2 - label_size.y / 2), textCol, label);

    PushStyleColor(ImGuiCol_Text, textCol);
    RenderText(pos + ImVec2(size.x / 2 - label_size.x / 2, size.y / 2 - label_size.y / 2), label);
    PopStyleColor();

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
    return pressed;
}


bool ImAdd::AcentButton(const char* label, const ImVec2& size_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);

    // Animate Colors
    static std::map<ImGuiID, DefaultStyles_State> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end())
    {
        anim.insert({ id, DefaultStyles_State() });
        it_anim = anim.find(id);
    }

    it_anim->second.FrameBg = ImLerp(it_anim->second.FrameBg, GetStyleColorVec4((held && hovered) ? ImGuiCol_SliderGrab : ImGuiCol_SliderGrabActive), 1.0f / fAnimationSpeed * ImGui::GetIO().DeltaTime);
    ImU32 frameCol = GetColorU32(it_anim->second.FrameBg);

    it_anim->second.Border = ImLerp(it_anim->second.Border, GetStyleColorVec4(hovered ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), 1.0f / fAnimationSpeed * ImGui::GetIO().DeltaTime);
    ImU32 borderCol = GetColorU32(it_anim->second.Border);

    // Render
    RenderNavHighlight(bb, id);
    //RenderFrame(bb.Min, bb.Max, frameCol, true, style.FrameRounding);

    window->DrawList->AddRectFilled(bb.Min, bb.Max, frameCol, style.FrameRounding);
    if (style.FrameBorderSize)
        window->DrawList->AddRect(bb.Min, bb.Max, borderCol, style.FrameRounding, 0, style.FrameBorderSize);

    if (g.LogEnabled)
        LogSetNextTextDecoration("[", "]");
    RenderTextClipped(bb.Min + style.FramePadding, bb.Max - style.FramePadding, label, NULL, &label_size, style.ButtonTextAlign, &bb);

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
    return pressed;
}

bool ImAdd::RadioButton(const char* label, int* v, int current_id, const ImVec2& size_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    bool active = *v == current_id;

    float border_size = style.FrameBorderSize;

    // Animate Colors
    static std::map<ImGuiID, DefaultStyles_State> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end())
    {
        anim.insert({ id, DefaultStyles_State() });
        it_anim = anim.find(id);
    }

    ImVec4 frameColDefault = GetStyleColorVec4(ImGuiCol_SliderGrabActive);
    ImVec4 frameColNull = frameColDefault; frameColNull.w = 0;
    it_anim->second.FrameBg = ImLerp(it_anim->second.FrameBg, active ? frameColDefault : frameColNull, 1.0f / fAnimationSpeed * ImGui::GetIO().DeltaTime);
    ImU32 frameCol = GetColorU32(it_anim->second.FrameBg);

    it_anim->second.Text = ImLerp(it_anim->second.Text, GetStyleColorVec4(active ? ImGuiCol_SliderGrab : (held || hovered) ? ImGuiCol_Text : ImGuiCol_TextDisabled), 1.0f / fAnimationSpeed * ImGui::GetIO().DeltaTime);
    ImU32 textCol = GetColorU32(it_anim->second.Text);

    // Render
    RenderNavHighlight(bb, id);

    window->DrawList->AddRectFilled(pos + ImVec2(0, size.y - 1), pos + size, frameCol, style.FrameRounding);

    window->DrawList->AddText(pos + ImVec2(size.x / 2 - label_size.x / 2, size.y / 2 - label_size.y / 2 + 1), textCol, label);

    if (pressed)
        *v = current_id;

    return pressed;
}

bool ImAdd::RadioButtonIcon(const char* str_id, const char* icon, const char* label, int* v, int current_id, const ImVec2& size_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(icon);
    const ImVec2 icon_size = CalcTextSize(icon, NULL, true);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(size_arg, icon_size.x + label_size.x + style.FramePadding.x * 3.0f, g.FontSize + style.FramePadding.y * 2.0f);

    const ImRect bb(pos, pos + size);
    ItemSize(size, style.FramePadding.y);
    if (!ItemAdd(bb, id))
        return false;

    // Behaviors
    bool hovered, held, active;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    active = *v == current_id;

    // Animation
    static std::map<ImGuiID, DefaultStyles_State> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end())
    {
        anim.insert({ id, DefaultStyles_State() });
        it_anim = anim.find(id);
    }

    // Colors
    ImVec4 frameColDefault = GetStyleColorVec4(ImGuiCol_FrameBg);
    ImVec4 frameColNull = frameColDefault; frameColNull.w = 0;
    it_anim->second.FrameBg = ImLerp(it_anim->second.FrameBg, active ? frameColDefault : frameColNull, 1.0f / fAnimationSpeed * ImGui::GetIO().DeltaTime);
    ImU32 frameCol = GetColorU32(it_anim->second.FrameBg);
    
    ImVec4 frameActiveColDefault = GetStyleColorVec4(ImGuiCol_SliderGrab);
    ImVec4 frameActiveColNull = frameActiveColDefault; frameActiveColNull.w = 0;
    it_anim->second.FrameBgActive = ImLerp(it_anim->second.FrameBgActive, active ? frameActiveColDefault : frameActiveColNull, 1.0f / fAnimationSpeed * ImGui::GetIO().DeltaTime);
    ImU32 frameActiveCol = GetColorU32(it_anim->second.FrameBgActive);
    
    ImVec4 borderColDefault = GetStyleColorVec4(ImGuiCol_Border);
    ImVec4 borderColNull = borderColDefault; borderColNull.w = 0;
    it_anim->second.Border = ImLerp(it_anim->second.Border, active ? borderColDefault : borderColNull, 1.0f / fAnimationSpeed * ImGui::GetIO().DeltaTime);
    ImU32 borderCol = GetColorU32(it_anim->second.Border);
    
    it_anim->second.Text = ImLerp(it_anim->second.Text, GetStyleColorVec4((active || (hovered && !held)) ? ImGuiCol_Text  : ImGuiCol_TextDisabled), 1.0f / fAnimationSpeed * ImGui::GetIO().DeltaTime);
    ImU32 textCol = GetColorU32(it_anim->second.Text);
    
    it_anim->second.TextDisabled = ImLerp(it_anim->second.TextDisabled, GetStyleColorVec4((active || (hovered && !held)) ? ImGuiCol_SliderGrab : ImGuiCol_TextDisabled), 1.0f / fAnimationSpeed * ImGui::GetIO().DeltaTime);
    ImU32 textDisabledCol = GetColorU32(it_anim->second.TextDisabled);
    
    // Render
    RenderNavHighlight(bb, id);

    window->DrawList->AddRectFilled(pos, pos + size, frameCol, style.FrameRounding);
    if (style.FrameBorderSize > 0) {
        window->DrawList->AddRect(pos, pos + size, borderCol, style.FrameRounding, 0, style.FrameBorderSize);
    }

    window->DrawList->AddText(pos + ImVec2(size.y / 2 - icon_size.y / 2 + 1, size.y / 2 - icon_size.y / 2 + 1), textDisabledCol, icon);
    window->DrawList->AddText(pos + ImVec2(size.y + 1, size.y / 2 - label_size.y / 2), textCol, label);
    
    // Click
    if (pressed) {
        *v = current_id;
    }

    return pressed;
}

struct select_state
{
    ImVec4 text;
    float opticaly;
};

bool ImAdd::Selectable(const char* label, bool selected, ImGuiSelectableFlags flags, const ImVec2& size_arg)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    ImGuiID id = window->GetID(label);
    ImVec2 label_size = CalcTextSize(label, NULL, true);
    ImVec2 size(size_arg.x != 0.0f ? size_arg.x : label_size.x, size_arg.y != 0.0f ? size_arg.y : label_size.y);
    ImVec2 pos = window->DC.CursorPos;
    pos.y += window->DC.CurrLineTextBaseOffset;
    ItemSize(size, 0.0f);

    const bool span_all_columns = (flags & ImGuiSelectableFlags_SpanAllColumns) != 0;
    const float min_x = span_all_columns ? window->ParentWorkRect.Min.x : pos.x;
    const float max_x = span_all_columns ? window->ParentWorkRect.Max.x : window->WorkRect.Max.x;
    if (size_arg.x == 0.0f || (flags & ImGuiSelectableFlags_SpanAvailWidth)) size.x = ImMax(label_size.x, max_x - min_x);

    const ImVec2 text_min = pos;
    const ImVec2 text_max(min_x + size.x, pos.y + size.y);

    ImRect bb(min_x, pos.y, text_max.x, text_max.y);
    if ((flags & ImGuiSelectableFlags_NoPadWithHalfSpacing) == 0)
    {
        const float spacing_x = span_all_columns ? 0.0f : style.ItemSpacing.x;
        const float spacing_y = style.ItemSpacing.y;
        const float spacing_L = IM_TRUNC(spacing_x * 0.50f);
        const float spacing_U = IM_TRUNC(spacing_y * 0.50f);
        bb.Min.x -= spacing_L;
        bb.Min.y -= spacing_U;
        bb.Max.x += (spacing_x - spacing_L);
        bb.Max.y += (spacing_y - spacing_U);
    }

    const float backup_clip_rect_min_x = window->ClipRect.Min.x;
    const float backup_clip_rect_max_x = window->ClipRect.Max.x;
    if (span_all_columns)
    {
        window->ClipRect.Min.x = window->ParentWorkRect.Min.x;
        window->ClipRect.Max.x = window->ParentWorkRect.Max.x;
    }

    const bool disabled_item = (flags & ImGuiSelectableFlags_Disabled) != 0;
    const bool item_add = ItemAdd(bb, id, NULL, disabled_item ? ImGuiItemFlags_Disabled : ImGuiItemFlags_None);
    if (span_all_columns)
    {
        window->ClipRect.Min.x = backup_clip_rect_min_x;
        window->ClipRect.Max.x = backup_clip_rect_max_x;
    }

    if (!item_add) return false;

    const bool disabled_global = (g.CurrentItemFlags & ImGuiItemFlags_Disabled) != 0;
    if (disabled_item && !disabled_global) BeginDisabled();

    if (span_all_columns && window->DC.CurrentColumns) PushColumnsBackground();
    else if (span_all_columns && g.CurrentTable) TablePushBackgroundChannel();

    ImGuiButtonFlags button_flags = 0;
    if (flags & ImGuiSelectableFlags_NoHoldingActiveID) { button_flags |= ImGuiButtonFlags_NoHoldingActiveId; }
    if (flags & ImGuiSelectableFlags_NoSetKeyOwner) { button_flags |= ImGuiButtonFlags_NoSetKeyOwner; }
    if (flags & ImGuiSelectableFlags_SelectOnClick) { button_flags |= ImGuiButtonFlags_PressedOnClick; }
    if (flags & ImGuiSelectableFlags_SelectOnRelease) { button_flags |= ImGuiButtonFlags_PressedOnRelease; }
    if (flags & ImGuiSelectableFlags_AllowDoubleClick) { button_flags |= ImGuiButtonFlags_PressedOnClickRelease | ImGuiButtonFlags_PressedOnDoubleClick; }
    if (flags & ImGuiSelectableFlags_AllowItemOverlap) { button_flags |= ImGuiButtonFlags_AllowItemOverlap; }

    const bool was_selected = selected;
    bool hovered, held, pressed = ButtonBehavior(bb, id, &hovered, &held, button_flags);

    if ((flags & ImGuiSelectableFlags_SelectOnNav) && g.NavJustMovedToId != 0 && g.NavJustMovedToFocusScopeId == g.CurrentFocusScopeId)
        if (g.NavJustMovedToId == id)  selected = pressed = true;

    // Update NavId when clicking or when Hovering (this doesn't happen on most widgets), so navigation can be resumed with gamepad/keyboard
    if (pressed || (hovered && (flags & ImGuiSelectableFlags_SetNavIdOnHover)))
    {
        if (!g.NavDisableMouseHover && g.NavWindow == window && g.NavLayer == window->DC.NavLayerCurrent)
        {
            SetNavID(id, window->DC.NavLayerCurrent, g.CurrentFocusScopeId, WindowRectAbsToRel(window, bb)); // (bb == NavRect)
            g.NavDisableHighlight = true;
        }
    }
    if (pressed) MarkItemEdited(id);

    if (selected != was_selected)  g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_ToggledSelection;


    if (g.NavId == id) RenderNavHighlight(bb, id, ImGuiNavHighlightFlags_TypeThin | ImGuiNavHighlightFlags_NoRounding);

    if (span_all_columns && window->DC.CurrentColumns) PopColumnsBackground();
    else if (span_all_columns && g.CurrentTable) TablePopBackgroundChannel();

    static std::map<ImGuiID, select_state> anim;
    select_state& state = anim[id];

    state.text = ImLerp(state.text, selected ? c::accent : c::elements::text, g.IO.DeltaTime * 6.f);
    state.opticaly = ImLerp(state.opticaly, selected ? 1.f : 0.f, g.IO.DeltaTime * 6.f);

    GetWindowDrawList()->AddRectFilled(bb.Min, bb.Max + ImVec2(0, 1), GetColorU32(ImGuiCol_ChildBg, state.opticaly), 2.f);

    RenderTextColor(text_min + ImVec2(1, 1), text_max, GetColorU32(state.text), label, ImVec2(0.f, 0.5f));

    if (pressed && (window->Flags & ImGuiWindowFlags_Popup) && !(flags & ImGuiSelectableFlags_DontClosePopups) && !(g.LastItemData.InFlags & ImGuiItemFlags_SelectableDontClosePopup)) CloseCurrentPopup();

    if (disabled_item && !disabled_global) EndDisabled();

    return pressed;
}

bool ImAdd::Selectable(const char* label, bool* p_selected, ImGuiSelectableFlags flags, const ImVec2& size_arg)
{
    if (ImAdd::Selectable(label, *p_selected, flags, size_arg))
    {
        *p_selected = !*p_selected;
        return true;
    }
    return false;
}

bool ImAdd::Checkbox(const char* label, bool* v)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    const float square_sz = GetFrameHeight() * 1.f;
    const ImVec2 pos = window->DC.CursorPos;
    const ImRect total_bb(pos, pos + ImVec2(square_sz + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f), label_size.y + style.FramePadding.y * 2.0f));
    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id))
    {
        IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags | ImGuiItemStatusFlags_Checkable | (*v ? ImGuiItemStatusFlags_Checked : 0));
        return false;
    }

    bool hovered, held;
    bool pressed = ButtonBehavior(total_bb, id, &hovered, &held);
    if (pressed)
    {
        *v = !(*v);
        MarkItemEdited(id);
    }
    // Animate Colors
    static std::map<ImGuiID, DefaultStyles_State> anim;
    auto it_anim = anim.find(id);

    if (it_anim == anim.end())
    {
        anim.insert({ id, DefaultStyles_State() });
        it_anim = anim.find(id);
    }

    ImVec4 frameColDefault = GetStyleColorVec4(ImGuiCol_SliderGrab);
    ImVec4 frameColNull = frameColDefault; frameColNull.w = 0;
    it_anim->second.FrameBg = ImLerp(it_anim->second.FrameBg, *v ? frameColDefault : frameColNull, 1.0f / fAnimationSpeed * ImGui::GetIO().DeltaTime);
    ImU32 frameCol = GetColorU32(it_anim->second.FrameBg);

    ImVec4 frameActiveColDefault = GetStyleColorVec4(ImGuiCol_FrameBg);
    ImVec4 frameActiveColNull = frameActiveColDefault; frameActiveColNull.w = 0;
    it_anim->second.FrameBgActive = ImLerp(it_anim->second.FrameBgActive, !*v ? frameActiveColDefault : frameActiveColNull, 1.0f / fAnimationSpeed * ImGui::GetIO().DeltaTime);
    ImU32 frameActiveCol = GetColorU32(it_anim->second.FrameBgActive);

    ImVec4 borderColDefault = GetStyleColorVec4(ImGuiCol_Border);
    ImVec4 borderColNull = borderColDefault; borderColNull.w = 0;
    it_anim->second.Border = ImLerp(it_anim->second.Border, !*v ? borderColDefault : borderColNull, 1.0f / fAnimationSpeed * ImGui::GetIO().DeltaTime);
    ImU32 borderCol = GetColorU32(it_anim->second.Border);

    // Render
    const ImRect check_bb(pos, pos + ImVec2(square_sz, square_sz));
    RenderNavHighlight(total_bb, id);

    RenderFrame(check_bb.Min, check_bb.Max, frameCol, false, style.FrameRounding);

    ImU32 check_col = GetColorU32(ImGuiCol_CheckMark);
    bool mixed_value = (g.LastItemData.InFlags & ImGuiItemFlags_MixedValue) != 0;
    if (mixed_value)
    {
        // Undocumented tristate/mixed/indeterminate checkbox (#2644)
        // This may seem awkwardly designed because the aim is to make ImGuiItemFlags_MixedValue supported by all widgets (not just checkbox)
        ImVec2 pad(ImMax(1.0f, IM_TRUNC(square_sz / 3.6f)), ImMax(1.0f, IM_TRUNC(square_sz / 3.6f)));
        window->DrawList->AddRectFilled(check_bb.Min + pad, check_bb.Max - pad, check_col, style.FrameRounding);
    }
    else if (*v)
    {
        const float pad = ImMax(1.0f, IM_TRUNC(square_sz / 4.0f));
        RenderCheckMark(window->DrawList, check_bb.Min + ImVec2(pad, pad), check_col, square_sz - pad * 2.0f);
        DrawShadowRectFallback(GetWindowDrawList(), check_bb.Min, check_bb.Max, GetColorU32(ImGuiCol_SliderGrab), c::ShadowThickness, 16.f);
    }

    window->DrawList->AddRectFilled(check_bb.Min, check_bb.Max, frameActiveCol, style.FrameRounding);
    if (style.FrameBorderSize > 0)
        window->DrawList->AddRect(check_bb.Min, check_bb.Max, borderCol, style.FrameRounding, 0, style.FrameBorderSize);

    ImVec2 label_pos = ImVec2(check_bb.Max.x + style.ItemInnerSpacing.x, check_bb.Min.y + style.FramePadding.y);
    if (label_size.x > 0.0f)
        RenderText(label_pos, label);

    return pressed;
}

bool ImAdd::SmallCheckbox(const char* label, bool* v)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    bool result = Checkbox(label, v);
    PopStyleVar();

    return result;
}

bool ImAdd::ColorEdit4(const char* label, float col[4])
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImVec4 col_v4(col[0], col[1], col[2], col[3]);

    ImGuiColorEditFlags flags = ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoSidePreview;

    BeginGroup();

    bool result = ColorButton(label, col_v4, flags, ImVec2(g.FontSize * 2, g.FontSize));
    if (result)
    {
        OpenPopup("picker");
    }
    if (BeginPopup("picker"))
    {
        ColorPicker4(label, col, flags);
        EndPopup();
    }

    SameLine(style.ItemInnerSpacing.x + g.FontSize * 2);
    Text(label);

    EndGroup();

    return result;
}

bool ImAdd::InputText(const char* label, const char* text, char* buf, size_t buf_size, float width, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImVec2 text_size = CalcTextSize(text, NULL, true);

    IM_ASSERT(!(flags & ImGuiInputTextFlags_Multiline)); // call InputTextMultiline()

    bool result = false;
    bool has_label = label_size.x > 0;

    const float w = CalcItemSize(ImVec2(width, 0), CalcItemWidth(), 0).x;

    BeginGroup();
    {
        if (has_label) {
            PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(style.ItemSpacing.x, style.ItemInnerSpacing.y));
            ImGui::Text(label);
        }
        {
            ImVec2 pos = GetCursorScreenPos();
            PushItemWidth(w);
            {
                result |= InputTextEx(std::string("##" + std::string(label)).c_str(), NULL, buf, (int)buf_size, ImVec2(0, 0), flags, callback, user_data);
            }
            PopItemWidth();

            if (text_size.x > 0)
            {
                if (!ImGui::IsItemActive() && !strlen(buf)) {
                    //ImGui::SameLine(pos.x);
                    ImGui::SetCursorScreenPos(pos + style.FramePadding);
                    ImGui::TextDisabled(text);
                }
            }
        }
        if (has_label) {
            PopStyleVar();
        }
    }
    EndGroup();

    return result;
}

void ImAdd::SeparatorText(const char* label)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = CalcItemSize(ImVec2(-0.1f, g.FontSize), label_size.x, g.FontSize);

    const ImRect total_bb(pos, pos + size);
    ItemSize(total_bb);
    if (!ItemAdd(total_bb, id)) {
        return;
    }

    window->DrawList->AddText(pos, GetColorU32(ImGuiCol_TextDisabled), label);
    window->DrawList->AddLine(pos + ImVec2(label_size.x + style.ItemInnerSpacing.x, size.y / 2), pos + ImVec2(size.x, size.y / 2), GetColorU32(ImGuiCol_Border));
}

struct begin_state
{
    ImVec4 background, text, outline;
    float open, alpha, combo_size = 0.f, shadow_opticaly;
    bool opened_combo = false, hovered = false;
    float arrow_roll;
};

static float CalcMaxPopupHeightFromItemCount(int items_count)
{
    ImGuiContext& g = *GImGui;
    if (items_count <= 0)
        return FLT_MAX;
    return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
}

int rotation_start_index;
void ImRotateStart()
{
    rotation_start_index = ImGui::GetWindowDrawList()->VtxBuffer.Size;
}

ImVec2 ImRotationCenter()
{
    ImVec2 l(FLT_MAX, FLT_MAX), u(-FLT_MAX, -FLT_MAX);

    const auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
    for (int i = rotation_start_index; i < buf.Size; i++)
        l = ImMin(l, buf[i].pos), u = ImMax(u, buf[i].pos);

    return ImVec2((l.x + u.x) / 2, (l.y + u.y) / 2);
}

void ImRotateEnd(float rad, ImVec2 center = ImRotationCenter())
{
    float s = sin(rad), c = cos(rad);
    center = ImRotate(center, s, c) - center;

    auto& buf = ImGui::GetWindowDrawList()->VtxBuffer;
    for (int i = rotation_start_index; i < buf.Size; i++)
        buf[i].pos = ImRotate(buf[i].pos, s, c) - center;
}

static const char* Items_ArrayGetter(void* data, int idx)
{
    const char* const* items = (const char* const*)data;
    return items[idx];
}

bool ImAdd::BeginCombo(const char* label, const char* description, const char* preview_value, int val, bool multi, ImGuiComboFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();

    g.NextWindowData.ClearFlags();
    if (window->SkipItems) return false;

    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const float w = ((GetContentRegionMax().x - style.WindowPadding.x));
    const float y_size = 50;



    const ImRect bb(window->DC.CursorPos + ImVec2(0, 0), window->DC.CursorPos + ImVec2(w, 50));
    const ImRect rect(window->DC.CursorPos + ImVec2(w - 160, 10), window->DC.CursorPos + ImVec2(w - 10, 40));

    const ImRect total_bb(bb.Min, bb.Max);
    ItemSize(bb, 0.f);

    if (!ItemAdd(bb, id, &bb)) return false;

    bool hovered, held, pressed = ButtonBehavior(bb, id, &hovered, &held);

    static std::map<ImGuiID, begin_state> anim;
    begin_state& state = anim[id];

    if (hovered && g.IO.MouseClicked[0] || state.opened_combo && g.IO.MouseClicked[0] && !state.hovered) state.opened_combo = !state.opened_combo;

    state.arrow_roll = ImLerp(state.arrow_roll, state.opened_combo ? -1.f : 1.f, g.IO.DeltaTime * 6.f);
    state.text = ImLerp(state.text, state.opened_combo ? c::elements::text_active : hovered ? c::elements::text_hov : c::elements::text, g.IO.DeltaTime * 6.f);
    state.combo_size = ImLerp(state.combo_size, state.opened_combo ? (val * 33) + 17 : 0.f, g.IO.DeltaTime * 12.f);




    //childbg
    GetWindowDrawList()->AddRectFilled(bb.Min, bb.Max, GetColorU32(ImGuiCol_ChildBg), c::roundings::childrounding);


    //Combo Preview
    GetWindowDrawList()->AddRectFilled(rect.Min, rect.Max, GetColorU32(c::background::filling), c::roundings::childrounding);

    RenderTextColor(rect.Min + ImVec2(10, 0), rect.Max, GetColorU32(ImGuiCol_Text), preview_value, ImVec2(0.0, 0.5));
    GetWindowDrawList()->AddRectFilledMultiColor(rect.Min, rect.Max, GetColorU32(ImGuiCol_WindowBg, 0.f), GetColorU32(ImGuiCol_WindowBg, 1.f), GetColorU32(ImGuiCol_WindowBg, 1.f), GetColorU32(ImGuiCol_WindowBg, 0.f));

    RenderTextColor(bb.Min + ImVec2(10, 0), bb.Max, GetColorU32(ImGuiCol_Text), label, ImVec2(0.0, 0.5));

    GetWindowDrawList()->AddRectFilled(rect.Max - ImVec2(y_size, y_size), rect.Max, GetColorU32(state.background), c::roundings::childrounding);

    if (!IsRectVisible(rect.Min, rect.Max + ImVec2(0, 2)))
    {
        state.opened_combo = false;
        state.combo_size = 0.f;
    }

    if (!state.opened_combo && state.combo_size < 2.f) return false;

    ImGui::SetNextWindowPos(ImVec2(rect.Min.x, rect.Max.y + 5));
    ImGui::SetNextWindowSize(ImVec2(rect.GetWidth(), state.combo_size));

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

    PushStyleColor(ImGuiCol_ChildBg, c::background::child);
    PushStyleColor(ImGuiCol_Border, c::elements::background_widget);
    PushStyleVar(ImGuiStyleVar_WindowRounding, c::elements::rounding);
    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
    PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);

    bool ret = Begin(label, NULL, window_flags);

    PopStyleVar(3);
    PopStyleColor(2);

    state.hovered = IsWindowHovered();

    if (multi && state.hovered && g.IO.MouseClicked[0]) state.opened_combo = false;

    return true;
}

void ImAdd::EndCombo()
{
    End();
}

void ImAdd::MultiCombo(const char* label, const char* description, bool variable[], const char* labels[], int count)
{
    ImGuiContext& g = *GImGui;

    std::string preview = "None";

    for (auto i = 0, j = 0; i < count; i++)
    {
        if (variable[i])
        {
            if (j)
                preview += (", ") + (std::string)labels[i];
            else
                preview = labels[i];

            j++;
        }
    }

    if (ImAdd::BeginCombo(label, description, preview.c_str(), count, NULL, NULL))
    {
        for (auto i = 0; i < count; i++)
        {
            PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(15, 15));
            PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
            ImAdd::Selectable(labels[i], &variable[i]);
            PopStyleVar(2);
        }
        End();
    }

    preview = ("None");
}

bool BeginComboPreview()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiComboPreviewData* preview_data = &g.ComboPreviewData;

    if (window->SkipItems || !(g.LastItemData.StatusFlags & ImGuiItemStatusFlags_Visible)) return false;

    IM_ASSERT(g.LastItemData.Rect.Min.x == preview_data->PreviewRect.Min.x && g.LastItemData.Rect.Min.y == preview_data->PreviewRect.Min.y);

    if (!window->ClipRect.Overlaps(preview_data->PreviewRect)) return false;

    preview_data->BackupCursorPos = window->DC.CursorPos;
    preview_data->BackupCursorMaxPos = window->DC.CursorMaxPos;
    preview_data->BackupCursorPosPrevLine = window->DC.CursorPosPrevLine;
    preview_data->BackupPrevLineTextBaseOffset = window->DC.PrevLineTextBaseOffset;
    preview_data->BackupLayout = window->DC.LayoutType;
    window->DC.CursorPos = preview_data->PreviewRect.Min + g.Style.FramePadding;
    window->DC.CursorMaxPos = window->DC.CursorPos;
    window->DC.LayoutType = ImGuiLayoutType_Horizontal;
    window->DC.IsSameLine = false;
    PushClipRect(preview_data->PreviewRect.Min, preview_data->PreviewRect.Max, true);

    return true;
}

void EndComboPreview()
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiComboPreviewData* preview_data = &g.ComboPreviewData;

    ImDrawList* draw_list = window->DrawList;
    if (window->DC.CursorMaxPos.x < preview_data->PreviewRect.Max.x && window->DC.CursorMaxPos.y < preview_data->PreviewRect.Max.y)
        if (draw_list->CmdBuffer.Size > 1)
        {
            draw_list->_CmdHeader.ClipRect = draw_list->CmdBuffer[draw_list->CmdBuffer.Size - 1].ClipRect = draw_list->CmdBuffer[draw_list->CmdBuffer.Size - 2].ClipRect;
            draw_list->_TryMergeDrawCmds();
        }
    PopClipRect();
    window->DC.CursorPos = preview_data->BackupCursorPos;
    window->DC.CursorMaxPos = ImMax(window->DC.CursorMaxPos, preview_data->BackupCursorMaxPos);
    window->DC.CursorPosPrevLine = preview_data->BackupCursorPosPrevLine;
    window->DC.PrevLineTextBaseOffset = preview_data->BackupPrevLineTextBaseOffset;
    window->DC.LayoutType = preview_data->BackupLayout;
    window->DC.IsSameLine = false;
    preview_data->PreviewRect = ImRect();
}

static const char* Items_SingleStringGetter(void* data, int idx)
{
    const char* items_separated_by_zeros = (const char*)data;
    int items_count = 0;
    const char* p = items_separated_by_zeros;
    while (*p)
    {
        if (idx == items_count)
            break;
        p += strlen(p) + 1;
        items_count++;
    }
    return *p ? p : NULL;
}

bool ImAdd::Combo(const char* label, const char* description, int* current_item, const char* (*getter)(void* user_data, int idx), void* user_data, int items_count, int popup_max_height_in_items)
{
    ImGuiContext& g = *GImGui;

    const char* preview_value = NULL;
    if (*current_item >= 0 && *current_item < items_count)
        preview_value = getter(user_data, *current_item);

    if (popup_max_height_in_items != -1 && !(g.NextWindowData.Flags & ImGuiNextWindowDataFlags_HasSizeConstraint))
        SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, CalcMaxPopupHeightFromItemCount(popup_max_height_in_items)));

    if (!ImAdd::BeginCombo(label, description, preview_value, items_count, ImGuiComboFlags_None, NULL)) return false;

    bool value_changed = false;
    PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(15, 15));
    for (int i = 0; i < items_count; i++)
    {
        const char* item_text = getter(user_data, i);
        if (item_text == NULL)
            item_text = "*Unknown item*";

        PushID(i);
        const bool item_selected = (i == *current_item);
        if (ImAdd::Selectable(item_text, item_selected) && *current_item != i)
        {
            value_changed = true;
            *current_item = i;
        }
        if (item_selected)
            SetItemDefaultFocus();
        PopID();
    }
    PopStyleVar();

    ImAdd::EndCombo();

    if (value_changed)
        MarkItemEdited(g.LastItemData.ID);

    return value_changed;
}

bool ImAdd::Combo(const char* label, const char* description, int* current_item, const char* const items[], int items_count, int height_in_items)
{
    const bool value_changed = ImAdd::Combo(label, description, current_item, Items_ArrayGetter, (void*)items, items_count, height_in_items);
    return value_changed;
}

bool ImAdd::Combo(const char* label, const char* description, int* current_item, const char* items_separated_by_zeros, int height_in_items)
{
    int items_count = 0;
    const char* p = items_separated_by_zeros;
    while (*p)
    {
        p += strlen(p) + 1;
        items_count++;
    }
    bool value_changed = ImAdd::Combo(label, description, current_item, Items_SingleStringGetter, (void*)items_separated_by_zeros, items_count, height_in_items);
    return value_changed;
}




struct DefaultAnims_State {
    int GrabWidth;
};

bool ImAdd::SliderScalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);

    char value_buf[64];
    const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    float w = CalcItemSize(ImVec2(-0.1f, 0), 0/*fixme*/, 0).x;
    w -= label_size.x > 0 ? 0 : CalcTextSize(value_buf).x + style.ItemInnerSpacing.x;

    ImVec2 pos = window->DC.CursorPos;

    const ImRect total_bb(pos, pos + ImVec2(w, g.FontSize + style.FramePadding.y * 4));
    const ImRect frame_bb(total_bb.Min + ImVec2(0, total_bb.GetHeight() - 6), total_bb.Max);

    ItemSize(total_bb, style.FramePadding.y);
    if (!ItemAdd(total_bb, id, &frame_bb))
        return false;

    if (format == NULL)
        format = DataTypeGetInfo(data_type)->PrintFmt;

    const bool hovered = ItemHoverable(frame_bb, 0);

    const bool input_requested_by_tabbing = (g.LastItemData.StatusFlags & ImGuiItemStatusFlags_FocusedByTabbing) != 0;
    const bool clicked = hovered && IsMouseClicked(0, id);
    const bool make_active = (input_requested_by_tabbing || clicked || g.NavActivateId == id);
    if (make_active && clicked)
        SetKeyOwner(ImGuiKey_MouseLeft, id);

    if (make_active)
    {
        SetActiveID(id, window);
        SetFocusID(id, window);
        FocusWindow(window);
        g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
    }

    ImRect grab_bb;
    const bool value_changed = SliderBehavior(frame_bb, id, data_type, p_data, p_min, p_max, format, NULL, &grab_bb);
    if (value_changed)
        MarkItemEdited(id);

    static std::unordered_map< ImGuiID, float > values;
    auto value = values.find(id);

    if (value == values.end()) {
        values.insert({ id, 0.f });
        value = values.find(id);
    }

    float percent = (grab_bb.Max.x - window->DC.CursorPos.x - frame_bb.Min.x + pos.x) / frame_bb.GetWidth();

    value->second = ImLerp(value->second, percent * frame_bb.GetWidth(), fAnimationSpeed);

    RenderFrame(total_bb.Min, total_bb.Max, GetColorU32(ImGuiCol_ChildBg), 1, style.FrameRounding);

    window->DrawList->AddRectFilled(frame_bb.Min, { grab_bb.Min.x, frame_bb.Max.y }, GetColorU32(ImGuiCol_SliderGrab, 0.3f), style.FrameRounding);
    
    DrawShadowRectFallback(GetWindowDrawList(), frame_bb.Min, ImVec2(frame_bb.Min.x + value->second, frame_bb.Max.y), GetColorU32(ImGuiCol_SliderGrab), c::ShadowThickness, 16.f);

    window->DrawList->AddRectFilled(frame_bb.Min, { frame_bb.Min.x + value->second, frame_bb.Max.y }, GetColorU32(ImGuiCol_SliderGrab), style.FrameRounding, ImDrawFlags_RoundCornersAll);

    // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
    window->DrawList->AddText(pos + ImVec2(w - CalcTextSize(value_buf).x - style.FramePadding.x * 2, style.FramePadding.y * 2), GetColorU32(ImGuiCol_TextDisabled), value_buf);

    if (label_size.x > 0.0f)
        window->DrawList->AddText(pos + style.FramePadding * 2, GetColorU32(ImGuiCol_Text), label);

    IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
    return value_changed;
}

bool ImAdd::SliderFloat(const char* label, float* v, float v_min, float v_max, const char* format)
{
    return SliderScalar(label, ImGuiDataType_Float, v, &v_min, &v_max, format);
}

bool ImAdd::SliderInt(const char* label, int* v, int v_min, int v_max, const char* format)
{
    return SliderScalar(label, ImGuiDataType_S32, v, &v_min, &v_max, format);
}



struct button_state1 //button_state1 because else object error  (already used in imgui)
{
    ImVec4 background, text, outline;
    float circle_size, delay_opticaly;
};

std::map<ImGuiID, button_state1> btn_anim;

bool ImAdd::Button(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags)
{
    ImGuiWindow* window = GetCurrentWindow();

    if (window->SkipItems) return false;

    std::string stripped = std::string(label);

    size_t poz = stripped.find("##");
    if (poz != std::string::npos)
        stripped = stripped.substr(0, poz);

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const ImVec2 label_size = CalcTextSize(stripped.c_str(), NULL, true), pos = window->DC.CursorPos;
    static bool circle_active = false;
    ImVec2 size = CalcItemSize(size_arg, label_size.x, label_size.y);


    const ImRect bb(pos, pos + size);

    ItemSize(size, 0.f);
    if (!ItemAdd(bb, id)) return false;

    bool hovered, held, pressed = ButtonBehavior(bb, id, &hovered, &held, flags);

    auto it_anim = btn_anim.find(id);

    if (it_anim == btn_anim.end())
    {
        btn_anim.insert({ id, button_state1() });
        it_anim = btn_anim.find(id);
    }

    it_anim->second.background = ImLerp(it_anim->second.background, IsItemActive() ? ImGuiCol_SliderGrab : ImColor(25, 25, 25, 255), g.IO.DeltaTime * 6.f);
    it_anim->second.outline = ImLerp(it_anim->second.outline, IsItemActive() ? ImGuiCol_SliderGrab : ImColor(25, 25, 25, 255), g.IO.DeltaTime * 6.f);

    if (IsItemClicked()) circle_active = true;

    it_anim->second.circle_size = ImLerp(it_anim->second.circle_size, circle_active ? 251.f : 0.f, g.IO.DeltaTime * 15.f);
    it_anim->second.delay_opticaly = ImLerp(it_anim->second.delay_opticaly, circle_active ? 1.f : 0.f, g.IO.DeltaTime * 7.f);


    if (it_anim->second.circle_size > 250) circle_active = false;

    ImGui::GetWindowDrawList()->AddRectFilled(bb.Min, bb.Max, GetColorU32(it_anim->second.outline), c::roundings::childrounding);

    //invisible_shadow(1.f, bb.Min, bb.Max, GetColorU32(c::accent), 500);

    ImGui::GetWindowDrawList()->AddRectFilled(bb.Min + ImVec2(1, 1), bb.Max - ImVec2(1, 1), GetColorU32(it_anim->second.background), c::roundings::childrounding);

    PushStyleVar(ImGuiStyleVar_Alpha, it_anim->second.delay_opticaly);
    PushClipRect(bb.Min, bb.Max, true);
    DrawShadowCircleFallback(GetWindowDrawList(), bb.Min + ImVec2(size_arg) / 2, 3 + it_anim->second.circle_size, GetColorU32(ImGuiCol_SliderGrab), 3 + it_anim->second.circle_size);
    PopClipRect();
    PopStyleVar();

    ImGui::GetWindowDrawList()->AddText(ImVec2(bb.Min.x + (size_arg.x - CalcTextSize(stripped.c_str()).x) / 2, bb.Max.y - CalcTextSize(stripped.c_str()).y - (size.y - CalcTextSize(stripped.c_str()).y) / 2), GetColorU32(ImGuiCol_Text), stripped.c_str());

    return pressed;
}

