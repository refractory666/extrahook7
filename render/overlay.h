#ifndef OVERLAY_H
#define OVERLAY_H

#include "includes.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

class UI
{
private:
	static ID3D11Device* pd3dDevice;
	static ID3D11DeviceContext* pd3dDeviceContext;
	static IDXGISwapChain* pSwapChain;
	static ID3D11RenderTargetView* pMainRenderTargetView;
	static bool bInit;
	static HWND hTargetWindow;
	static BOOL bTargetSet;
	static DWORD dTargetPID;

	static bool CreateDeviceD3D(HWND hWnd);
	static void CleanupDeviceD3D();
	static void CreateRenderTarget();
	static void CleanupRenderTarget();
	static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static BOOL EnumWind(HWND hWindow, LPARAM lPrams);
	static void GetWindow();
	static void MoveWindow(HWND hCurrentProcessWindow);
	static BOOL IsWindowFocus(HWND hCurrentProcessWindow);
	static BOOL EnumAllWind(HWND hWindow, LPARAM lPrams);
	static BOOL IsWindowCloaked(HWND hCurrentWindow);
	static BOOL IsWindowAlive();

public:
	static HMODULE hCurrentModule;
	static void GetProcessName(LPSTR lpProcessName, DWORD dPID);
	static BOOL IsWindowValid(HWND hCurrentWindow);

	struct WindowItem
	{
		HWND CurrentWindow;
		char CurrentWindowTitle[125];
		char CurrentProcessName[125];
	};

	static void Render();
	static BOOL IsWindowTargeted();
	static void GetAllWindow(std::vector<WindowItem>* vWindowList);
	static void SetTargetWindow(HWND hWindow);
	static void UpdWindowInfo();
	static RECT GetOverlayRect();

	// Load image from memory (e.g. Photon avatar bytes). Returns ImTextureID for use with AddImage/AddImageRounded, or nullptr on failure.
	static void* LoadImageTextureFromMemory(const unsigned char* data, size_t size);
};

#endif
