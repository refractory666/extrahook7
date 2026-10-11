#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_groups.h"
#include <string>
#include <map>

using namespace ImGui;

void ImGrp::Checkbox(const char* label, bool* v)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    BeginChild(label, ImVec2(0, GetFrameHeight() + style.FramePadding.y * 2), false);
    {
        SetCursorPos(style.FramePadding * 2);
        Text(label);

        SetCursorPos(ImVec2(GetWindowWidth() - GetFontSize() - style.FramePadding.x * 2.5f, style.FramePadding.y * 2.f));
        //SetCursorPos(ImVec2(GetWindowWidth() - GetFontSize() - style.FramePadding.x * 2.5f, style.FramePadding.y * 1.5f));
        PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 0));
        PushStyleVar(ImGuiStyleVar_FramePadding, style.FramePadding / 2);
        //ImAdd::Checkbox(label, v);
        ImAdd::SmallCheckbox(label, v);
        PopStyleVar();
        PopStyleColor();
    }
    EndChild();
}


struct color_state
{
    bool active = false;
    bool hovered = false;
    float alpha = 0.f;
    float clr_offset = 0.f;
};

void ImGrp::ColorEdit4(const char* label, float col[4])
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImVec2 label_size = CalcTextSize(label, NULL, true);

    const ImVec4 col_v4(col[0], col[1], col[2], col[3]);
    int cr = IM_F32_TO_INT8_SAT(col[0]), cg = IM_F32_TO_INT8_SAT(col[1]), cb = IM_F32_TO_INT8_SAT(col[2]), ca = IM_F32_TO_INT8_SAT(col[3]);
    const std::string popup_name = std::string(label) + "##ColorEdit4";

    char hex_label[64] = "";
    sprintf_s(hex_label, sizeof(hex_label), "#%02X%02X%02X%02X", cr, cg, cb, ca);

    static std::map<ImGuiID, color_state> anim;
    auto id = GetID(label);
    auto it_anim = anim.find(id);

    if (it_anim == anim.end())
    {
        anim.insert({ id, color_state() });
        it_anim = anim.find(id);
    }

    BeginChild(label, ImVec2(0, GetFrameHeight() + style.FramePadding.y * 2), false);
    {
        SetCursorPos(style.FramePadding * 2);
        Text(label);

        SetCursorPos(ImVec2(GetWindowWidth() - GetFrameHeight() * 2 - style.FramePadding.x - CalcTextSize(hex_label).x, style.FramePadding.y * 2));
        TextDisabled(hex_label);

        SetCursorPos(ImVec2(GetWindowWidth() - GetFrameHeight() * 2 + style.FramePadding.x * 0.5f, style.FramePadding.y * 1.5f));
        PushStyleVar(ImGuiStyleVar_FramePadding, style.FramePadding / 2);
        if (ImGui::ColorButton(("##" + std::string(label)).c_str(), col_v4, ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoTooltip, ImVec2(GetFrameHeight() * 2, GetFrameHeight())))
        {
            OpenPopup(popup_name.c_str());
        }
        PopStyleVar();

        bool popup_open = IsPopupOpen(popup_name.c_str());
        if (popup_open && !it_anim->second.active)
        {
            it_anim->second.active = true;
            it_anim->second.alpha = 0.0f; // Start animation from fully transparent
        }

        if (!popup_open && it_anim->second.active)
        {
            it_anim->second.active = false;
        }

        if (it_anim->second.active)
        {
            it_anim->second.alpha = ImClamp(it_anim->second.alpha + (4.f * g.IO.DeltaTime * (it_anim->second.active ? 1.f : -1.f)), 0.f, 1.f);
        }
        else
        {
            it_anim->second.alpha = ImClamp(it_anim->second.alpha - (4.f * g.IO.DeltaTime * (it_anim->second.active ? 1.f : -1.f)), 0.f, 1.f);
        }

        if (it_anim->second.alpha >= 0.01f);
        {
            PushStyleVar(ImGuiStyleVar_Alpha, it_anim->second.alpha);
            if (BeginPopup(popup_name.c_str()))
            {
                ColorPicker4(label, col, ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoSidePreview);
                EndPopup();
            }
            PopStyleVar();
        }
    }
    EndChild();
}
