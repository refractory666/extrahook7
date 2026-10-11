#pragma once

#include "imgui.h"

namespace font
{
    inline ImFont* mainfont = nullptr;
}

namespace c
{
    inline static ImVec2 size = ImVec2(700, 550);
    inline float ShadowThickness = 0;
    inline ImVec4 accent = ImVec4(1.f, 0.f, 0.f, 1.f);

    namespace background
    {
        inline ImVec4 filling = ImVec4(21/255.f, 21/255.f, 21/255.f, 1.f);
        inline ImVec4 child = ImVec4(25/255.f, 25/255.f, 25/255.f, 1.f);
        inline ImVec4 stroke = ImVec4(24/255.f, 26/255.f, 36/255.f, 1.f);
        inline ImVec2 size = ImVec2(900, 515);
        inline float rounding = 6;
    }

    namespace roundings
    {
        inline float windowrounding = 3;
        inline float childrounding = 2;
        inline float framerounding = 2;
        inline float popuprounding = 2;
        inline float grabrounding = 2;
        inline float tabrounding = 2;
        inline float scrollbarrounding = 1;
    }

    namespace elements
    {
        inline ImVec4 border = ImVec4(31/255.f, 31/255.f, 31/255.f, 1.f);
        inline ImVec4 seperator = ImVec4(31/255.f, 31/255.f, 31/255.f, 1.f);
        inline ImVec4 scrollbar_background = ImVec4(31/255.f, 31/255.f, 31/255.f, 0.f);
        inline ImVec4 scrollbar_grabbed = ImVec4(25/255.f, 25/255.f, 25/255.f, 0.f);
        inline ImVec4 scrollbar_grab_hovered = ImVec4(25/255.f, 25/255.f, 25/255.f, 0.f);
        inline ImVec4 scrollbar_active = ImVec4(25/255.f, 25/255.f, 25/255.f, 0.f);
        inline ImVec4 button = ImVec4(21/255.f, 21/255.f, 21/255.f, 1.f);
        inline ImVec4 button_active = ImVec4(21/255.f, 21/255.f, 21/255.f, 200/255.f);
        inline ImVec4 button_hovered = ImVec4(21/255.f, 21/255.f, 21/255.f, 170/255.f);
        inline ImVec4 mark = ImVec4(0, 0, 0, 1.f);
        inline ImVec4 stroke = ImVec4(28/255.f, 26/255.f, 37/255.f, 1.f);
        inline ImVec4 background = ImVec4(255/255.f, 15/255.f, 17/255.f, 1.f);
        inline ImVec4 background_widget = ImVec4(21/255.f, 23/255.f, 26/255.f, 1.f);
        inline ImVec4 text_active = ImVec4(97/255.f, 97/255.f, 97/255.f, 1.f);
        inline ImVec4 text_hov = ImVec4(255/255.f, 255/255.f, 255/255.f, 1.f);
        inline ImVec4 text = ImVec4(255/255.f, 255/255.f, 255/255.f, 1.f);
        inline ImVec4 text_disabled = ImVec4(97/255.f, 97/255.f, 97/255.f, 1.f);
        inline float rounding = 4;
    }

    namespace tab
    {
        inline ImVec4 tab_active = ImVec4(22/255.f, 22/255.f, 30/255.f, 1.f);
        inline ImVec4 border = ImVec4(14/255.f, 14/255.f, 15/255.f, 1.f);
    }
}
