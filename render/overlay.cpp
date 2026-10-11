#include "overlay.h"
#include "menu.h"
#include "features/cheat/esp.h"
#include "features/cheat/aimbot.h"
#include "features/cheat/misc.h"
#include "features/cheat/triggerbot.h"
#include "features/cheat/map_service.h"
#include "glow_shader.h"
#include "memory.h"
#include "font.h"
#include "icons.h"
#include "settings_menu.h"
#include "fonts/Poppins_Medium.h"
#include "fonts/IconsFontAwesome6.h"
#include "fonts/IconsFontAwesome6Brands.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../backend/imgui/stb_image.h"

static ImTextureID LoadImageTexture(ID3D11Device* device, const char* path)
{
	int w = 0, h = 0, ch = 0;
	unsigned char* pixels = stbi_load(path, &w, &h, &ch, 4);
	if (!pixels || w <= 0 || h <= 0) return nullptr;
	ID3D11ShaderResourceView* srv = nullptr;
	{
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = (UINT)w;
		desc.Height = (UINT)h;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		D3D11_SUBRESOURCE_DATA sub = {};
		sub.pSysMem = pixels;
		sub.SysMemPitch = (UINT)(w * 4);
		sub.SysMemSlicePitch = 0;
		ID3D11Texture2D* tex = nullptr;
		HRESULT hr = device->CreateTexture2D(&desc, &sub, &tex);
		stbi_image_free(pixels);
		if (FAILED(hr) || !tex) return nullptr;
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		hr = device->CreateShaderResourceView(tex, &srvDesc, &srv);
		tex->Release();
		if (FAILED(hr) || !srv) return nullptr;
	}
	return (ImTextureID)srv;
}

static ImTextureID LoadImageTextureFromMemoryImpl(ID3D11Device* device, const unsigned char* data, size_t size)
{
	if (!data || size == 0 || !device) return nullptr;
	int w = 0, h = 0, ch = 0;
	unsigned char* pixels = stbi_load_from_memory(data, (int)size, &w, &h, &ch, 4);
	if (!pixels || w <= 0 || h <= 0) return nullptr;
	ID3D11ShaderResourceView* srv = nullptr;
	{
		D3D11_TEXTURE2D_DESC desc = {};
		desc.Width = (UINT)w;
		desc.Height = (UINT)h;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		D3D11_SUBRESOURCE_DATA sub = {};
		sub.pSysMem = pixels;
		sub.SysMemPitch = (UINT)(w * 4);
		sub.SysMemSlicePitch = 0;
		ID3D11Texture2D* tex = nullptr;
		HRESULT hr = device->CreateTexture2D(&desc, &sub, &tex);
		stbi_image_free(pixels);
		if (FAILED(hr) || !tex) return nullptr;
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		hr = device->CreateShaderResourceView(tex, &srvDesc, &srv);
		tex->Release();
		if (FAILED(hr) || !srv) return nullptr;
	}
	return (ImTextureID)srv;
}

void* UI::LoadImageTextureFromMemory(const unsigned char* data, size_t size)
{
	return pd3dDevice ? (void*)LoadImageTextureFromMemoryImpl(pd3dDevice, data, size) : nullptr;
}

ID3D11Device* UI::pd3dDevice = nullptr;
ID3D11DeviceContext* UI::pd3dDeviceContext = nullptr;
IDXGISwapChain* UI::pSwapChain = nullptr;
ID3D11RenderTargetView* UI::pMainRenderTargetView = nullptr;
bool UI::bInit = false;
HWND UI::hTargetWindow = nullptr;
BOOL UI::bTargetSet = FALSE;
DWORD UI::dTargetPID = 0;

HMODULE UI::hCurrentModule = nullptr;

bool UI::CreateDeviceD3D(HWND hWnd)
{
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

    const UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &pSwapChain, &pd3dDevice, &featureLevel, &pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &pSwapChain, &pd3dDevice, &featureLevel, &pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    RECT cr = {};
    GetClientRect(hWnd, &cr);
    int w = cr.right - cr.left;
    int h = cr.bottom - cr.top;
    if (w <= 0 || h <= 0) {
        w = GetSystemMetrics(SM_CXSCREEN);
        h = GetSystemMetrics(SM_CYSCREEN);
    }
    glow_shader::init(pd3dDevice, w, h);
    return true;
}

/**
    @brief : Function that create the render target.
**/
void UI::CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    if (pBackBuffer != nullptr)
    {
        pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &pMainRenderTargetView);
        pBackBuffer->Release();
    }
}

/**
    @brief : Function that release the render target.
**/
void UI::CleanupRenderTarget()
{
    if (pMainRenderTargetView)
    {
        pMainRenderTargetView->Release();
        pMainRenderTargetView = nullptr;
    }
}

/**
    @brief : Function that release the D3D11 device.
**/
void UI::CleanupDeviceD3D()
{
    glow_shader::shutdown();
    CleanupRenderTarget();
    if (pSwapChain)
    {
        pSwapChain->Release();
        pSwapChain = nullptr;
    }

    if (pd3dDeviceContext)
    {
        pd3dDeviceContext->Release();
        pd3dDeviceContext = nullptr;
    }

    if (pd3dDevice)
    {
        pd3dDevice->Release();
        pd3dDevice = nullptr;
    }
}

#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0 // From Windows SDK 8.1+ headers
#endif
#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_EXCLUDEFROMCAPTURE 0x00000011 // Win10 2004+: exclude from screen capture (OBS, etc.)
#endif

/**
    @brief : Window message handler (https://learn.microsoft.com/en-us/windows/win32/api/winuser/nc-winuser-wndproc).
**/
LRESULT WINAPI UI::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (pd3dDevice != nullptr && wParam != SIZE_MINIMIZED)
        {
            CleanupRenderTarget();
            pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
            glow_shader::resize((int)LOWORD(lParam), (int)HIWORD(lParam));
        }
        return 0;

    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU)
            return 0;
        break;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;

    default:
        break;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}

/**
    @brief : Function that create the overlay window and more.
**/
void UI::Render()
{
    ImGui_ImplWin32_EnableDpiAwareness();

    WNDCLASSEX wc;
    wc.cbClsExtra = NULL;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.cbWndExtra = NULL;
    wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(0, 0, 0));
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpfnWndProc = WndProc;
    wc.lpszClassName = _T("D3D11 Overlay ImGui");
    wc.lpszMenuName = nullptr;
    wc.style = CS_VREDRAW | CS_HREDRAW;

    ::RegisterClassEx(&wc);
    const HWND hwnd = ::CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_NOACTIVATE,
        wc.lpszClassName, _T("D3D11 Overlay ImGui"),
        WS_POPUP,
        0, 0,
        GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
        nullptr, nullptr, wc.hInstance, nullptr);

    SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
    const MARGINS margin = { -1, 0, 0, 0 };
    DwmExtendFrameIntoClientArea(hwnd, &margin);

    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClass(wc.lpszClassName, wc.hInstance);
        return;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;


    ImGuiStyle& style = ImGui::GetStyle();

    ImGui::StyleColorsDark();

    const HMONITOR monitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO info = {};
    info.cbSize = sizeof(MONITORINFO);
    GetMonitorInfo(monitor, &info);
    const int monitor_height = info.rcMonitor.bottom - info.rcMonitor.top;

    ImFont* font_default = nullptr;
    ImFontConfig cfg = {};
    font_default = io.Fonts->AddFontFromMemoryCompressedTTF(Poppins_Medium_compressed_data, (int)Poppins_Medium_compressed_size, 20.f, &cfg, io.Fonts->GetGlyphRangesDefault());
    if (!font_default)
        font_default = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    if (!font_default)
        font_default = io.Fonts->AddFontDefault();
    if (font_default)
        io.FontDefault = font_default;
    font::mainfont = font_default;
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_16_FA, 0 };
    static const ImWchar icons_ranges_brands[] = { ICON_MIN_FAB, ICON_MAX_16_FAB, 0 };
    ImFontConfig fa_config = {};
    fa_config.MergeMode = true;
    fa_config.PixelSnapH = true;
    io.Fonts->AddFontFromMemoryCompressedTTF(fa6_solid_compressed_data, (int)fa6_solid_compressed_size, 14.f, &fa_config, icons_ranges);
    io.Fonts->AddFontFromMemoryCompressedTTF(fa_brands_400_compressed_data, (int)fa_brands_400_compressed_size, 14.f, &fa_config, icons_ranges_brands);

    // ESP: Verdana 12 for nicknames — PixelSnapH + low oversample for sharp text
    ImFontConfig font_cfg_name = {};
    font_cfg_name.OversampleH = 1;
    font_cfg_name.OversampleV = 1;
    font_cfg_name.PixelSnapH = true;
    esp::g_esp_font_name = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\verdana.ttf", 12.0f, &font_cfg_name);
    ImFontConfig font_cfg_hp = {};
    font_cfg_hp.FontDataOwnedByAtlas = false;
    esp::g_esp_font_hp = io.Fonts->AddFontFromMemoryTTF(smallPixel, (int)sizeof(smallPixel), 10.0f, &font_cfg_hp, io.Fonts->GetGlyphRangesDefault());
    if (!esp::g_esp_font_hp)
        esp::g_esp_font_hp = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\consola.ttf", 10.0f);
    if (!esp::g_esp_font_hp)
        esp::g_esp_font_hp = font_default;

    // Weapon icons: load at 2x size and draw at target size so we never scale up (avoids blur)
    ImFontConfig font_cfg_weapons = {};
    font_cfg_weapons.FontDataOwnedByAtlas = false;
    font_cfg_weapons.OversampleH = 1;
    font_cfg_weapons.OversampleV = 1;
    font_cfg_weapons.PixelSnapH = true;
    esp::g_esp_font_weapons = io.Fonts->AddFontFromMemoryTTF(Weapons, (int)sizeof(Weapons), 28.0f, &font_cfg_weapons, io.Fonts->GetGlyphRangesDefault());

    ImFontConfig font_cfg_icon = {};
    font_cfg_icon.FontDataOwnedByAtlas = false;
    font_cfg_icon.OversampleH = 1;
    font_cfg_icon.OversampleV = 1;
    font_cfg_icon.PixelSnapH = true;
    g_menu_icon_font = io.Fonts->AddFontFromMemoryTTF(icomoon, (int)sizeof(icomoon), 36.0f, &font_cfg_icon, io.Fonts->GetGlyphRangesDefault());

    ImGui::GetIO().IniFilename = nullptr;

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(pd3dDevice, pd3dDeviceContext);

    const ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    bInit = true;

    bool bDone = false;

    // FPS limiter: reduce CPU load on AMD / Bluestacks (target 60 FPS, minimal yield when behind)
    const int kTargetFps = 60;
    const double kTargetFrameMs = 1000.0 / (double)kTargetFps;
    LARGE_INTEGER perfFreq = {}, frameStart = {};
    QueryPerformanceFrequency(&perfFreq);
    QueryPerformanceCounter(&frameStart);

    // Throttle window rect updates (avoid EnumWindows every frame)
    static uint64_t s_lastWindowUpdate = 0;
    const uint64_t kWindowUpdateIntervalMs = 100;

    while (!bDone)
    {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                bDone = true;
        }
        if (bDone)
            break;

        static int s_heavy_frame_count = 0;
        uint64_t nowMs = (uint64_t)GetTickCount64();
        if (nowMs - s_lastWindowUpdate >= kWindowUpdateIntervalMs)
        {
            UpdWindowInfo();
            s_lastWindowUpdate = nowMs;
        }
        RECT overlayRect = GetOverlayRect();
        int width = overlayRect.right - overlayRect.left;
        int height = overlayRect.bottom - overlayRect.top;
        SetWindowPos(hwnd, HWND_TOPMOST, overlayRect.left, overlayRect.top, width, height, SWP_NOACTIVATE);

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // Один раз через 1 сек после старта: libunity + player_manager
        {
            static uint64_t s_loop_start = 0;
            static bool s_auto_init_triggered = false;
            if (s_loop_start == 0) s_loop_start = (uint64_t)GetTickCount64();
            if (!s_auto_init_triggered && ((uint64_t)GetTickCount64() - s_loop_start) >= 1000)
            {
                Drawing::try_auto_init_esp_once();
                s_auto_init_triggered = true;
            }
        }

        mem::Memory* pMem = (mem::Memory*)Drawing::get_memory();
        size_t unityBase = Drawing::get_unity_base();
        if (pMem && pMem->IsReady() && unityBase != 0)
        {
            static uint64_t s_last_pm_refresh = 0;
            static uint64_t s_last_players_update = 0;
            uint64_t now = (uint64_t)GetTickCount64();
            if (now - s_last_pm_refresh >= 2000)
            {
                esp::refresh_client_world_cache(pMem, unityBase);
                s_last_pm_refresh = now;
            }
            if (esp::g_client_world != 0)
            {
                esp::update_local_player(pMem);
                esp::update_view_matrix(pMem);
                const bool throttle = (s_heavy_frame_count >= 3);
                const bool do_update = !throttle || (now - s_last_players_update >= 20);
                if (do_update)
                {
                    esp::update_players(pMem);
                    esp::update_grenades(pMem);
                    s_last_players_update = now;
                }
                aimbot::run(pMem);
                misc::run(pMem, unityBase);
                ImVec2 ds = ImGui::GetIO().DisplaySize;
                triggerbot::run(pMem, ds.x, ds.y);
            }
            if (esp::g_client_world != 0)
            {
            ImDrawList* bg = ImGui::GetBackgroundDrawList();
            if (bg)
            {
                ImVec2 ds = ImGui::GetIO().DisplaySize;
                esp::render(bg, ds.x, ds.y);
                aimbot::draw_fov(bg, ds.x, ds.y);
                map_service::Update();
                map_service::Render(bg, esp::g_view_matrix, ds.x, ds.y);
            }
            }
        }

        Drawing::draw();

        ImGui::EndFrame();
        if (Drawing::is_active())
            SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW);
        else
            SetWindowLong(hwnd, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW);

        // OBS Bypass: exclude overlay from screen capture (OBS, Game Bar, etc.)
        SetWindowDisplayAffinity(hwnd, Drawing::b_obs_bypass ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE);

        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        pd3dDeviceContext->OMSetRenderTargets(1, &pMainRenderTargetView, nullptr);
        pd3dDeviceContext->ClearRenderTargetView(pMainRenderTargetView, clear_color_with_alpha);
        {
            static std::vector<glow_vertex> s_glow_tris;
            esp::get_hitbox_glow_triangles(s_glow_tris);
            ImVec2 ds = ImGui::GetIO().DisplaySize;
            int width = (int)ds.x, height = (int)ds.y;
            if (!s_glow_tris.empty() && esp::g_esp_hitbox_glow && width > 0 && height > 0)
            {
                float r = esp::g_esp_glow_color[0], g = esp::g_esp_glow_color[1], b = esp::g_esp_glow_color[2], a = esp::g_esp_glow_color[3];
                glow_shader::render(pd3dDeviceContext, s_glow_tris, width, height, r, g, b, a, esp::g_esp_glow_radius);
                glow_shader::draw_to_render_target(pd3dDeviceContext, pMainRenderTargetView, width, height);
            }
        }
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        pSwapChain->Present(1, 0);

        // Frame rate limiter + адаптивный троттлинг: при тяжёлых кадрах реже обновляем игроков (для AMD)
        LARGE_INTEGER frameEnd = {};
        QueryPerformanceCounter(&frameEnd);
        double elapsedMs = (double)(frameEnd.QuadPart - frameStart.QuadPart) * 1000.0 / (double)perfFreq.QuadPart;
        if (elapsedMs > 22.0) s_heavy_frame_count = (s_heavy_frame_count < 10) ? s_heavy_frame_count + 1 : 10;
        else if (elapsedMs < 18.0) s_heavy_frame_count = (s_heavy_frame_count > 0) ? s_heavy_frame_count - 1 : 0;
        double sleepMs = kTargetFrameMs - elapsedMs;
        if (sleepMs > 0.5)
        {
            if (sleepMs > 20.0) sleepMs = 20.0;
            Sleep((DWORD)(sleepMs));
        }
        else if (elapsedMs < kTargetFrameMs * 1.5)
            Sleep(1);
        QueryPerformanceCounter(&frameStart);
    }

    bInit = false;

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClass(wc.lpszClassName, wc.hInstance);

#ifdef _WINDLL
    CreateThread(nullptr, NULL, (LPTHREAD_START_ROUTINE)FreeLibrary, hCurrentModule, NULL, nullptr);
#else
    TerminateProcess(GetModuleHandleA(nullptr), 0);
#endif
}

static const char* const TARGET_PROCESS_NAME = "HD-Player.exe";
static RECT s_wndRect = { 0 };

static BOOL IsProcessHDPlayer(DWORD procID)
{
    if (procID == 0) return FALSE;
    char procName[125] = { 0 };
    UI::GetProcessName(procName, procID);
    return _stricmp(procName, TARGET_PROCESS_NAME) == 0;
}

static BOOL CALLBACK EnumChildWindowsProc(HWND hwnd, LPARAM lParam)
{
    std::vector<HWND>* windows = reinterpret_cast<std::vector<HWND>*>(lParam);
    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    if (processId == 0) return TRUE;
    if (!IsProcessHDPlayer(processId)) return TRUE;
    windows->push_back(hwnd);
    return TRUE;
}

BOOL CALLBACK UI::EnumWind(const HWND hWindow, const LPARAM lParam)
{
    std::vector<HWND>* windows = reinterpret_cast<std::vector<HWND>*>(lParam);
    EnumChildWindows(hWindow, EnumChildWindowsProc, lParam);
    return TRUE;
}

void UI::GetWindow()
{
    std::vector<HWND> windows;
    EnumWindows(EnumWind, reinterpret_cast<LPARAM>(&windows));
    if (!windows.empty())
        SetTargetWindow(windows[0]);
}

void UI::UpdWindowInfo()
{
    if (hTargetWindow == nullptr || !IsWindow(hTargetWindow))
        GetWindow();
    if (hTargetWindow != nullptr && IsWindow(hTargetWindow))
        GetWindowRect(hTargetWindow, &s_wndRect);
}

RECT UI::GetOverlayRect()
{
    RECT r = { 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
    if (hTargetWindow != nullptr && IsWindow(hTargetWindow))
        r = s_wndRect;
    return r;
}

void UI::MoveWindow(const HWND hCurrentProcessWindow)
{
    if (hTargetWindow == nullptr) return;
    RECT rect;
    GetWindowRect(hTargetWindow, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    SetWindowPos(hCurrentProcessWindow, HWND_TOPMOST, rect.left, rect.top, width, height, SWP_NOACTIVATE);
}

BOOL UI::IsWindowFocus(const HWND hCurrentProcessWindow)
{
    const HWND hForeground = GetForegroundWindow();
    if (!hForeground)
        return FALSE;

    DWORD fgPid = 0;
    GetWindowThreadProcessId(hForeground, &fgPid);
    char fgProcName[125] = { 0 };
    GetProcessName(fgProcName, fgPid);
    if (_stricmp(fgProcName, TARGET_PROCESS_NAME) != 0)
    {
        SetWindowLong(hCurrentProcessWindow, GWL_EXSTYLE, WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW);
        return FALSE;
    }
    return TRUE;
}

BOOL UI::IsWindowTargeted()
{
    return bTargetSet;
}

/**
    @brief : Function that clear the current window list and enumerate all windows.
    @param vWindowList : pointer of the WindowItem vector.
**/
void UI::GetAllWindow(std::vector<WindowItem>* vWindowList)
{
    vWindowList->clear();
    EnumWindows(EnumAllWind, (LPARAM)vWindowList);
}

/**
    @brief : Callback function that retrive all the valid window and get processname, pid and window title.
             (https://learn.microsoft.com/fr-fr/windows/win32/api/winuser/nf-winuser-enumwindows)
**/
BOOL CALLBACK UI::EnumAllWind(const HWND hWindow, const LPARAM lPrams)
{
    if (!IsWindowValid(hWindow))
        return TRUE;

    WindowItem CurrentWindowItem = { hWindow, 0, 0 };
    DWORD procID;

    GetWindowTextA(hWindow, CurrentWindowItem.CurrentWindowTitle, sizeof(CurrentWindowItem.CurrentWindowTitle));

    if (strlen(CurrentWindowItem.CurrentWindowTitle) == 0)
        return TRUE;

    GetWindowThreadProcessId(hWindow, &procID);
    GetProcessName(CurrentWindowItem.CurrentProcessName, procID);

    const auto vWindowList = (std::vector<WindowItem>*)lPrams;

    vWindowList->push_back(CurrentWindowItem);

    return TRUE;
}


/**
    @brief : Function that retrieve the process name from the PID.
    @param lpProcessName : pointer to the string that store the process name.
    @param dPID : PID of the process.
**/
void UI::GetProcessName(const LPSTR lpProcessName, const DWORD dPID)
{
    char lpCurrentProcessName[125];

    PROCESSENTRY32 ProcList{};
    ProcList.dwSize = sizeof(ProcList);

    const HANDLE hProcList = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcList == INVALID_HANDLE_VALUE)
        return;

    if (!Process32First(hProcList, &ProcList))
        return;


    if (ProcList.th32ProcessID == dPID)
    {
        wcstombs_s(nullptr, lpCurrentProcessName, ProcList.szExeFile, sizeof(lpCurrentProcessName));
        strncpy_s(lpProcessName, sizeof(lpCurrentProcessName), lpCurrentProcessName, sizeof(lpCurrentProcessName));
        return;
    }

    while (Process32Next(hProcList, &ProcList))
    {
        if (ProcList.th32ProcessID == dPID)
        {
            wcstombs_s(nullptr, lpCurrentProcessName, ProcList.szExeFile, sizeof(lpCurrentProcessName));
            strncpy_s(lpProcessName, sizeof(lpCurrentProcessName), lpCurrentProcessName, sizeof(lpCurrentProcessName));
            return;
        }
    }
}

/**
    @brief : Function that check if a window is valid.
    @param  hCurrentWindow : window to be tested.
    @retval : TRUE if the window is valid else FALSE.
**/
BOOL UI::IsWindowValid(const HWND hCurrentWindow)
{
    DWORD styles, ex_styles;
    RECT rect;

    if (!IsWindowVisible(hCurrentWindow) ||
        (IsIconic(hCurrentWindow) || IsWindowCloaked(hCurrentWindow)))
        return FALSE;

    GetClientRect(hCurrentWindow, &rect);
    styles = (DWORD)GetWindowLongPtr(hCurrentWindow, GWL_STYLE);
    ex_styles = (DWORD)GetWindowLongPtr(hCurrentWindow, GWL_EXSTYLE);

    if (ex_styles & WS_EX_TOOLWINDOW)
        return FALSE;
    if (styles & WS_CHILD)
        return FALSE;
    if (rect.bottom == 0 || rect.right == 0)
        return FALSE;

    return TRUE;
}

/**
    @brief : Function that check if a window is cloacked.
    @param  hCurrentWindow : window to be tested.
    @retval : TRUE if the window is cloacked else FALSE.
**/
BOOL UI::IsWindowCloaked(const HWND hCurrentWindow)
{
    DWORD cloaked;
    const HRESULT hr = DwmGetWindowAttribute(hCurrentWindow, DWMWA_CLOAKED, &cloaked,
        sizeof(cloaked));
    return SUCCEEDED(hr) && cloaked;
}

void UI::SetTargetWindow(const HWND hWindow)
{
    hTargetWindow = hWindow;
    SetForegroundWindow(hTargetWindow);
    GetWindowThreadProcessId(hTargetWindow, &dTargetPID);
    bTargetSet = TRUE;
}

/**
    @brief : Function that look if the targeted window has been closed.
    @retval : TRUE if the function is still up else FALSE.
**/
BOOL UI::IsWindowAlive()
{
    DWORD dCurrentPID;

    if (hTargetWindow == nullptr)
        return FALSE;

    if (!IsWindow(hTargetWindow))
        return FALSE;

    GetWindowThreadProcessId(hTargetWindow, &dCurrentPID);

    if (dCurrentPID != dTargetPID)
        return FALSE;

    return TRUE;
}