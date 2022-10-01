#include <d3d9.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/directx.hpp>
#include <tchar.h>
#include <dwmapi.h>
#include "imgui-1.88/imgui.h"
#include "imgui-1.88/backends/imgui_impl_dx9.h"
#include "imgui-1.88/backends/imgui_impl_win32.h"
using namespace std;
using namespace cv;

#define PROJ_NAME "DatasetCollector"
#define CAM_H 300
#define CAM_W 300

#define WINDOW_H 730
#define WINDOW_W 520



LPDIRECT3DDEVICE9       g_Device = NULL;
D3DPRESENT_PARAMETERS   g_PresentParams;
LPDIRECT3DTEXTURE9		g_Frame;
LPDIRECT3DTEXTURE9		g_ResultFrame;

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
	if (g_Device != NULL && wParam != SIZE_MINIMIZED)
	{
		ImGui_ImplDX9_InvalidateDeviceObjects();
		g_PresentParams.BackBufferWidth = LOWORD(lParam);
		g_PresentParams.BackBufferHeight = HIWORD(lParam);
		HRESULT hr = g_Device->Reset(&g_PresentParams);
		if (hr == D3DERR_INVALIDCALL)
			IM_ASSERT(0);
		ImGui_ImplDX9_CreateDeviceObjects();
	}
	return 0;
	case WM_SYSCOMMAND:
	if ((wParam & 0xfff0) == SC_KEYMENU)
		return 0;
	break;
	case WM_DESTROY:
	PostQuitMessage(0);
	return 0;
	break;
	
	}
	return DefWindowProc(hWnd, msg, wParam, lParam);
}




int main()
{

	VideoCapture cap(0);

	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T(PROJ_NAME), NULL };
	RegisterClassEx(&wc);
	HWND hwnd = CreateWindow(_T(PROJ_NAME), _T(PROJ_NAME), WS_OVERLAPPEDWINDOW, 200, 200, WINDOW_H, WINDOW_W, NULL, NULL, wc.hInstance, NULL);
	LPDIRECT3D9 pD3D;

	if ((pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
	{
		UnregisterClass(_T(PROJ_NAME), wc.hInstance);
		return 0;
	}
	ZeroMemory(&g_PresentParams, sizeof(g_PresentParams));
	g_PresentParams.Windowed = TRUE;
	g_PresentParams.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_PresentParams.BackBufferFormat = D3DFMT_UNKNOWN;
	g_PresentParams.EnableAutoDepthStencil = TRUE;
	g_PresentParams.AutoDepthStencilFormat = D3DFMT_D16;
	g_PresentParams.PresentationInterval = D3DPRESENT_INTERVAL_ONE;

	if (pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hwnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_PresentParams, &g_Device) < 0)
	{
		pD3D->Release();
		UnregisterClass(_T(PROJ_NAME), wc.hInstance);
		return 0;
	}

	ImGui::CreateContext();
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX9_Init(g_Device);

	
	MSG msg;
	ZeroMemory(&msg, sizeof(msg));
	ShowWindow(hwnd, SW_SHOWDEFAULT);
	UpdateWindow(hwnd);

	
	if (FAILED(g_Device->CreateTexture(CAM_W, CAM_H, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &g_Frame, NULL))) {


		g_Device->Release();
		pD3D->Release();
		UnregisterClass(_T(PROJ_NAME), wc.hInstance);
		return 0;
	}
	if (FAILED(g_Device->CreateTexture(CAM_W, CAM_H, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &g_ResultFrame, NULL))) {


		g_Device->Release();
		pD3D->Release();
		UnregisterClass(_T(PROJ_NAME), wc.hInstance);
		return 0;
	}

	Mat CamFrame;
	while (msg.message != WM_QUIT)
	{
		Mat Frame;
		cap.read(CamFrame);
		resize(CamFrame, Frame, { CAM_W,CAM_H });

		cv::cvtColor(Frame, Frame, cv::COLOR_RGB2RGBA);

		D3DLOCKED_RECT Rect;
		g_Frame->LockRect(0, &Rect, 0, 0);
		BYTE* pDst = (BYTE*)Rect.pBits;
		
		cv::Mat m(CAM_H, CAM_W, CV_8UC4, Rect.pBits, Rect.Pitch);

		Frame.copyTo(m);
		
		g_Frame->UnlockRect(0);

		if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			continue;
		}
		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		

		ImGui::SetNextWindowSize({ WINDOW_H, WINDOW_W });
		ImGui::SetNextWindowPos({0,0});
		ImGui::Begin("Main", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar| ImGuiWindowFlags_NoMove);

		ImGui::NewLine();
		ImGui::NewLine();
		ImGui::SameLine(30.f);
		ImGui::Image(g_Frame, { 300,300 }, {0,0}, { 1,1 }, { 1,1,1,1 },ImColor(0.f,1.f,1.f,1.f));
		ImGui::SameLine(0.f,50.f);
		ImGui::Image(g_ResultFrame, { 300,300 }, { 0,0 }, { 1,1 }, { 1,1,1,1 }, ImColor(0.f, 1.f, 1.f, 1.f));

		ImGui::NewLine();
		ImGui::NewLine();
		ImGui::SameLine(30.f);

		if (ImGui::Button("Capture", { 200.f,0.f })) {

			D3DLOCKED_RECT Rect;
			g_ResultFrame->LockRect(0, &Rect, 0, 0);
			BYTE* pDst = (BYTE*)Rect.pBits;

			cv::Mat m(CAM_H, CAM_W, CV_8UC4, Rect.pBits, Rect.Pitch);

			Frame.copyTo(m);

			g_ResultFrame->UnlockRect(0);
		}
		/*
		if (ImGui::Button("Clear", { 200.f,0.f })) {

			D3DLOCKED_RECT Rect;
			g_ResultFrame->LockRect(0, &Rect, 0, 0);
			BYTE* pDst = (BYTE*)Rect.pBits;
			cv::Mat m(CAM_H, CAM_W, CV_8UC4, Rect.pBits, Rect.Pitch);
			memset(m.data, NULL, m.dataend - m.data);
			g_ResultFrame->UnlockRect(0);
		}*/
		ImGui::SameLine(380.f);
		ImGui::Button("Select directory", { 200.f,0.f });

		ImGui::NewLine(); ImGui::SameLine(30.f);
		ImGui::Button("Save image", { 200.f,0.f });
		ImGui::SameLine(380.f);
		ImGui::Button("Open image", { 200.f,0.f });

		ImGui::NewLine(); ImGui::SameLine(30.f);
		
		ImGui::Button("Save image and mask", { 200.f,0.f });
		ImGui::SameLine(380.f);
		ImGui::Button("Clear mask", { 200.f,0.f });

		ImGui::NewLine(); ImGui::SameLine(30.f);
		static char UserName[60];
		ImGui::PushItemWidth(200.f);
		ImGui::Text("User name");
		ImGui::SameLine(380.f);
		ImGui::Text("Brush size");

		ImGui::NewLine(); ImGui::SameLine(30.f);
		ImGui::InputText("##Username", UserName,sizeof(UserName));
		ImGui::SameLine(380.f);
		static float BrushSize = 10.f;
		ImGui::SliderFloat("##Brush size", &BrushSize, 10.f, 50.f);



		ImGui::PopItemWidth();


		ImGui::End();

		if (g_Device->BeginScene() >= 0)
		{
			ImGui::Render();
			ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
			g_Device->EndScene();
		}

		g_Device->Present(NULL, NULL, NULL, NULL);



	}

	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	g_Device->Release();
	pD3D->Release();
	UnregisterClass(_T(PROJ_NAME), wc.hInstance);



	return 0;
}
