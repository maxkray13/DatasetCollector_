#include "stdafx.h"
#include "Main.h"
#include "CamHandler.h"
#include "DrawImpl.h"
using namespace std;
using namespace cv;

CamHandler* Camera;
LPDIRECT3DDEVICE9       g_Device;
D3DPRESENT_PARAMETERS   g_PresentParams;

std::vector<Paint> Mask;
size_t Mask_Idx = 0u;;

char UserName[60];
float BrushSize = 10.f;

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
		if (Camera) Camera->BeginReset();
		HRESULT hr = g_Device->Reset(&g_PresentParams);
		if (hr == D3DERR_INVALIDCALL)
			IM_ASSERT(0);

		ImGui_ImplDX9_CreateDeviceObjects();
		if (Camera) Camera->AfterReset();
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






void CbBrush(const ImDrawList* parent_list, const ImDrawCmd* cmd)
{
	GetOriginValue(D3DRS_COLORWRITEENABLE);
	GetOriginValue(D3DRS_SRGBWRITEENABLE);
	GetOriginValue(D3DRS_MULTISAMPLEANTIALIAS);
	GetOriginValue(D3DRS_CULLMODE);
	GetOriginValue(D3DRS_LIGHTING);
	GetOriginValue(D3DRS_ZENABLE);
	GetOriginValue(D3DRS_ALPHABLENDENABLE);
	GetOriginValue(D3DRS_ALPHATESTENABLE);
	GetOriginValue(D3DRS_BLENDOP);
	GetOriginValue(D3DRS_SRCBLEND);
	GetOriginValue(D3DRS_DESTBLEND);
	GetOriginValue(D3DRS_SCISSORTESTENABLE);


	DWORD o_FVF;
	g_Device->GetFVF(&o_FVF);

	g_Device->SetRenderState(D3DRS_COLORWRITEENABLE, 0xffffffff);
	g_Device->SetRenderState(D3DRS_SRGBWRITEENABLE, false);
	g_Device->SetRenderState(D3DRS_MULTISAMPLEANTIALIAS, false);
	g_Device->SetRenderState(D3DRS_CULLMODE, D3DCULL_CCW);
	g_Device->SetRenderState(D3DRS_LIGHTING, false);
	g_Device->SetRenderState(D3DRS_ZENABLE, false);
	g_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
	g_Device->SetRenderState(D3DRS_ALPHATESTENABLE, false);
	g_Device->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
	g_Device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	g_Device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
	g_Device->SetRenderState(D3DRS_SCISSORTESTENABLE, true);

	g_Device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);

	D3DRECT WndRect{};
	WndRect.x1 = 31;
	WndRect.x2 = 1310;
	WndRect.y1 = 43;
	WndRect.y2 = 763;

	g_Device->SetScissorRect((RECT*)&WndRect);
	const auto MousePos = ImGui::GetIO().MousePos;

	Circle(MousePos.x, MousePos.y, BrushSize, 0.f, 360.f, 16, 0xFFFF0000);

	if (MousePos.x > WndRect.x1 && MousePos.x< WndRect.x2 && MousePos.y > WndRect.y1 && MousePos.y < WndRect.y2) {

		if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {

			Mask.push_back({ MousePos,BrushSize });
		}
	}

	for (auto& it : Mask) {

		Circle(it.Pos.x, it.Pos.y, it.Sz, 0.f, 360.f, 16, 0xFFFF0000);
	}

	Render(g_Device);

	g_Device->SetFVF(o_FVF);
	SetOriginValue(D3DRS_COLORWRITEENABLE);
	SetOriginValue(D3DRS_SRGBWRITEENABLE);
	SetOriginValue(D3DRS_MULTISAMPLEANTIALIAS);
	SetOriginValue(D3DRS_CULLMODE);
	SetOriginValue(D3DRS_LIGHTING);
	SetOriginValue(D3DRS_ZENABLE);
	SetOriginValue(D3DRS_ALPHABLENDENABLE);
	SetOriginValue(D3DRS_ALPHATESTENABLE);
	SetOriginValue(D3DRS_BLENDOP);
	SetOriginValue(D3DRS_SRCBLEND);
	SetOriginValue(D3DRS_DESTBLEND);
	SetOriginValue(D3DRS_SCISSORTESTENABLE);




}
int main()
{

	WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, _T(PROJ_NAME), NULL };
	RegisterClassEx(&wc);
	HWND hwnd = CreateWindow(_T(PROJ_NAME), _T(PROJ_NAME), (WS_OVERLAPPED | WS_SYSMENU | WS_MINIMIZEBOX), 200, 200, WINDOW_H, WINDOW_W, NULL, NULL, wc.hInstance, NULL);
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

	Camera = new CamHandler(0);

	Camera->WatchCam();

	while (msg.message != WM_QUIT)
	{

		Camera->UpdateCamTexture();

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
		ImGui::SetNextWindowPos({ 0,0 });
		ImGui::Begin("Main", NULL, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove);
		ImGui::Text("%f", ImGui::GetIO().Framerate);
		ImGui::NewLine();
		ImGui::NewLine();
		ImGui::SameLine(30.f);
		ImGui::Image(Camera->GetResultFrameTexture(), { 1280,720 }, { 0,0 }, { 1,1 }, { 1,1,1,1 }, ImColor(0.f, 1.f, 1.f, 1.f));

		ImGui::SameLine(0.f, 50.f);
		ImGui::Image(Camera->GetCamTexture(), { 200,200 }, { 0,0 }, { 1,1 }, { 1,1,1,1 }, ImColor(0.f, 1.f, 1.f, 1.f));

		ImGui::SetCursorPos({ 1360.f,275.f });

		if (ImGui::Button("Capture", { 200.f,0.f })) {

			Camera->SetResultFrameFromCam();
		}
		ImGui::SetCursorPos({ 1360.f,300.f });

		if (ImGui::Button("Save image", { 200.f,0.f })) {

			Camera->SaveImg(UserName);
		}

		ImGui::SetCursorPos({ 1360.f,325.f });

		if (ImGui::Button("Save mask", { 200.f,0.f })) {
			Camera->SaveImgMsk(UserName);
		}

		ImGui::SetCursorPos({ 1360.f,350.f });
		if (ImGui::Button("Open image", { 200.f,0.f })) {
			static OPENFILENAMEA ofn;
			static char szFile[256];
			ZeroMemory(szFile, sizeof(szFile));
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = NULL;
			ofn.lpstrFile = szFile;
			ofn.lpstrFile[0] = '\0';
			ofn.nMaxFile = sizeof(szFile);
			ofn.lpstrFilter = ".jpg";
			ofn.nFilterIndex = 0;
			ofn.lpstrFileTitle = NULL;
			ofn.nMaxFileTitle = 0;
			ofn.lpstrInitialDir = NULL;
			if (GetOpenFileNameA(&ofn)) {
				Camera->LoadImg(szFile);
				auto FileName = std::filesystem::path(szFile).stem().generic_string();

				if (FileName.find('_') != std::string::npos) {

					sprintf_s(UserName, "%s", std::string(FileName, 0, FileName.find_last_of('_')).c_str());
				}
				else {

					ZeroMemory(UserName, sizeof(UserName));
				}

			}
		}
		//ImGui::SetCursorPos({ 1360.f,375.f });
		//ImGui::Button("Select directory", { 200.f,0.f });

		ImGui::PushItemWidth(200.f);

		ImGui::SetCursorPos({ 1360.f,375.f });
		ImGui::Text("User name");
		ImGui::SetCursorPos({ 1360.f,400.f });

		ImGui::InputText("##Username", UserName, sizeof(UserName));


		ImGui::SetCursorPos({ 1360.f,425.f });
		ImGui::Text("Brush size");

		ImGui::SetCursorPos({ 1360.f,450.f });
		ImGui::SliderFloat("##Brush size", &BrushSize, 1.f, 50.f);
		ImGui::SetCursorPos({ 1360.f,475.f });

		if (ImGui::Button("Clear mask", { 200.f,0.f })) {

			Mask.clear();


		}
		//	ImGui::GetWindowDrawList()->AddCircleFilled({ 1460.f,600}, BrushSize, ImColor(255,0,0));

		ImGui::PopItemWidth();

		//ImGui::GetWindowDrawList()->AddCallback(CbBrush, NULL);
		ImGui::End();

		if (g_Device->BeginScene() >= 0)
		{
			ImGui::Render();
			ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());


			CbBrush(NULL, NULL);

			g_Device->EndScene();

		}




		g_Device->Present(NULL, NULL, NULL, NULL);
		g_Device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.f, NULL);

		std::this_thread::sleep_for(1ms);
	}

	delete Camera;

	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
	g_Device->Release();
	pD3D->Release();
	UnregisterClass(_T(PROJ_NAME), wc.hInstance);



	return 0;
}
