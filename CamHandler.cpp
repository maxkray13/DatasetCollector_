#include "stdafx.h"
#include "CamHandler.h"
#include "DrawImpl.h"
#include "Main.h"

CamHandler::CamHandler(int CamIdx)
{
	Cap = cv::VideoCapture(CamIdx);
	if (!Cap.isOpened())
		throw std::runtime_error("!Cap.isOpened()");

	if (FAILED(g_Device->CreateTexture(CAM_W, CAM_H, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &Frame, NULL)))
		throw std::runtime_error("Failed to create Frame");

	if (FAILED(g_Device->CreateTexture(CAM_W, CAM_H, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &ResultFrame, NULL)))
		throw std::runtime_error("ResultFrame");

	
	if (FAILED(g_Device->CreateTexture(CAM_W, CAM_H, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &BBFrame, NULL)))
		throw std::runtime_error("BBFrame");

}

CamHandler::~CamHandler()
{
	StopThread.exchange(true);
	Future.get();

	if (Frame) Frame->Release();
	if (ResultFrame) ResultFrame->Release();
	if (BBFrame) BBFrame->Release();
	Cap.release();
}

void CamHandler::WatchCam()
{

	Future = std::async(std::launch::async, [&]()->void {

		while (!StopThread.load()) {

			if (!CameraUpdated.load()&&Cap.grab()) {

				FrameLock.lock();
				Cap.retrieve(CapBuffer);

				cv::resize(CapBuffer, CapBuffer, { CAM_W,CAM_H });

				cv::cvtColor(CapBuffer, CapBuffer, cv::COLOR_RGB2RGBA);
				FrameLock.unlock();
				CameraUpdated.exchange(true);
			}
			std::this_thread::sleep_for(32ms);
		}

		
	});
}

void CamHandler::UpdateCamTexture()
{

	D3DLOCKED_RECT Rect;

	if (CameraUpdated.load()) {

		if (!FrameLock.try_lock()) {

			return;
		}

		if (Frame->LockRect(0, &Rect, 0, 0) == D3D_OK) {

			cv::Mat m(CAM_H, CAM_W, CV_8UC4, Rect.pBits, Rect.Pitch);

			CapBuffer.copyTo(m);

			Frame->UnlockRect(0);
		}

		FrameLock.unlock();
		CameraUpdated.exchange(false);
	}

}

const LPDIRECT3DTEXTURE9 CamHandler::GetCamTexture()
{
	return Frame;
}

const LPDIRECT3DTEXTURE9 CamHandler::GetResultFrameTexture()
{
	return ResultFrame;
}

void CamHandler::SetResultFrameFromCam()
{
	D3DLOCKED_RECT Rect;
	D3DLOCKED_RECT Rect1;

	if (ResultFrame->LockRect(0, &Rect, 0, 0) == D3D_OK) {

		if (Frame->LockRect(0, &Rect1, 0, 0) == D3D_OK) {
			cv::Mat m(CAM_H, CAM_W, CV_8UC4, Rect1.pBits, Rect1.Pitch);
			cv::Mat m1(CAM_H, CAM_W, CV_8UC4, Rect.pBits, Rect.Pitch);

			m.copyTo(m1);
			Frame->UnlockRect(0);
		}

		ResultFrame->UnlockRect(0);
	}
}
void CamHandler::SaveImg(std::string User)
{
	
	using namespace std::filesystem;

	char szPath[MAX_PATH];

	GetModuleFileNameA(NULL, szPath, MAX_PATH);

	auto OutputPath = path{ szPath }.parent_path() / "" / path("raw_data/");
	if (!exists(OutputPath)) {

		create_directory(OutputPath);
	}
	int Idx = -999;
	if (User.size()) {

		for (auto& it : recursive_directory_iterator(OutputPath)) {
			
			if (User.compare(0u, User.length() -1u , it.path().stem().generic_string(),0u, User.length() - 1u) == 0) {
				
				const int FileIdx = std::atoi(std::string(it.path().stem().generic_string(), User.length() + 1u, 1u).c_str());
				if (FileIdx >= Idx)
					Idx = FileIdx + 1u;
			}
		}
		OutputPath += path(User + "_" + std::to_string(Idx == -999? 0 : Idx) + ".jpg");
	}
	else {

		for (auto& it : recursive_directory_iterator(OutputPath)) {

			const int FileIdx = std::atoi(it.path().stem().generic_string().c_str());
			if (FileIdx >= Idx)
				Idx = FileIdx + 1u;
		}

		OutputPath += path(std::to_string(Idx == -999 ? 0 : Idx) + ".jpg");
	}
	
	D3DXSaveTextureToFileW(OutputPath.c_str(), D3DXIFF_JPG, ResultFrame, NULL);
}

void CamHandler::SaveImgMsk(std::string User)
{
	using namespace std::filesystem;

	char szPath[MAX_PATH];

	GetModuleFileNameA(NULL, szPath, MAX_PATH);

	auto OutputPath = path{ szPath }.parent_path() / "" / path("ready_data/");
	if (!exists(OutputPath)) {

		create_directory(OutputPath);
	}
	int Idx = -999;
	if (User.size()) {

		for (auto& it : recursive_directory_iterator(OutputPath)) {

			if (User.compare(0u, User.length() - 1u, it.path().stem().generic_string(), 0u, User.length() - 1u) == 0) {

				const int FileIdx = std::atoi(std::string(it.path().stem().generic_string(), User.length() + 1u, 1u).c_str());
				if (FileIdx >= Idx)
					Idx = FileIdx + 1u;
			}
		}
		OutputPath += path(User + "_" + std::to_string(Idx == -999 ? 0 : Idx) + ".jpg");
	}
	else {

		for (auto& it : recursive_directory_iterator(OutputPath)) {

			const int FileIdx = std::atoi(it.path().stem().generic_string().c_str());
			if (FileIdx >= Idx)
				Idx = FileIdx + 1u;
		}

		OutputPath += path(std::to_string(Idx == -999 ? 0 : Idx) + ".jpg");
	}
	D3DRECT WndRect{};
	WndRect.x1 = 31;
	WndRect.x2 = 1310;
	WndRect.y1 = 43;
	WndRect.y2 = 763;
	IDirect3DSurface9* pbbSurface = NULL;
	
	if (!FAILED(g_Device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pbbSurface))) {

		IDirect3DSurface9* MaskSurf = NULL;

		if (!FAILED(g_Device->CreateRenderTarget(CAM_W, CAM_H, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, false, &MaskSurf, NULL))) {

			auto ret = g_Device->SetRenderTarget(0, MaskSurf);
			

			
			IDirect3DVertexDeclaration9* vertDec = NULL; IDirect3DVertexShader9* vertShader = NULL;
			g_Device->GetVertexDeclaration(&vertDec);
			g_Device->GetVertexShader(&vertShader);
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


			DWORD D3DSAMP_ADDRESSU_o;
			g_Device->GetSamplerState(NULL, D3DSAMP_ADDRESSU, &D3DSAMP_ADDRESSU_o);
			DWORD D3DSAMP_ADDRESSV_o;
			g_Device->GetSamplerState(NULL, D3DSAMP_ADDRESSV, &D3DSAMP_ADDRESSV_o);
			DWORD D3DSAMP_ADDRESSW_o;
			g_Device->GetSamplerState(NULL, D3DSAMP_ADDRESSW, &D3DSAMP_ADDRESSW_o);
			DWORD D3DSAMP_SRGBTEXTURE_o;
			g_Device->GetSamplerState(NULL, D3DSAMP_SRGBTEXTURE, &D3DSAMP_SRGBTEXTURE_o);
			DWORD D3DTSS_COLOROP_o;
			g_Device->GetTextureStageState(0, D3DTSS_COLOROP, &D3DTSS_COLOROP_o);
			DWORD D3DTSS_COLORARG1_o;
			g_Device->GetTextureStageState(0, D3DTSS_COLORARG1, &D3DTSS_COLORARG1_o);
			DWORD D3DTSS_COLORARG2_o;
			g_Device->GetTextureStageState(0, D3DTSS_COLORARG2, &D3DTSS_COLORARG2_o);
			DWORD D3DTSS_ALPHAOP_o;
			g_Device->GetTextureStageState(0, D3DTSS_ALPHAOP, &D3DTSS_ALPHAOP_o);
			DWORD D3DTSS_ALPHAARG1_o;
			g_Device->GetTextureStageState(0, D3DTSS_ALPHAARG1, &D3DTSS_ALPHAARG1_o);
			DWORD D3DTSS_ALPHAARG2_o;
			g_Device->GetTextureStageState(0, D3DTSS_ALPHAARG2, &D3DTSS_ALPHAARG2_o);
			DWORD D3DSAMP_MINFILTER_o;
			g_Device->GetSamplerState(NULL, D3DSAMP_MINFILTER, &D3DSAMP_MINFILTER_o);
			DWORD D3DSAMP_MAGFILTER_o;
			g_Device->GetSamplerState(NULL, D3DSAMP_MAGFILTER, &D3DSAMP_MAGFILTER_o);
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
			g_Device->SetSamplerState(NULL, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
			g_Device->SetSamplerState(NULL, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
			g_Device->SetSamplerState(NULL, D3DSAMP_ADDRESSW, D3DTADDRESS_WRAP);
			g_Device->SetSamplerState(NULL, D3DSAMP_SRGBTEXTURE, NULL);
			g_Device->SetPixelShader(NULL);
			g_Device->SetVertexShader(NULL);
			g_Device->SetTexture(NULL, NULL);
			g_Device->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
			g_Device->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
			g_Device->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
			g_Device->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
			g_Device->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
			g_Device->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
			g_Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			g_Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			g_Device->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);


			g_Device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0, 0, 0), 1.f, NULL);

			for (auto& it : Mask) {
			
				Circle(it.Pos.x - WndRect.x1, it.Pos.y - WndRect.y1, it.Sz, 0.f, 360.f, 16, 0xFFFFFFFF);
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



			g_Device->SetSamplerState(NULL, D3DSAMP_ADDRESSU, D3DSAMP_ADDRESSU_o);
			g_Device->SetSamplerState(NULL, D3DSAMP_ADDRESSV, D3DSAMP_ADDRESSV_o);
			g_Device->SetSamplerState(NULL, D3DSAMP_ADDRESSW, D3DSAMP_ADDRESSW_o);
			g_Device->SetSamplerState(NULL, D3DSAMP_SRGBTEXTURE, D3DSAMP_SRGBTEXTURE_o);

			g_Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DSAMP_MINFILTER_o);
			g_Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DSAMP_MAGFILTER_o);

			g_Device->SetVertexDeclaration(vertDec);
			g_Device->SetVertexShader(vertShader);


			ret = D3DXSaveSurfaceToFileW(OutputPath.c_str(), D3DXIFF_JPG, MaskSurf, NULL, NULL);
			ret = g_Device->SetRenderTarget(0, pbbSurface);
			MaskSurf->Release();
		}

		pbbSurface->Release();
	}
	/*
	D3DRECT WndRect{};
	WndRect.x1 = 31;
	WndRect.x2 = 1310;
	WndRect.y1 = 43;
	WndRect.y2 = 763;

	RECT kk{};
	kk.left = 31;
	kk.right = 1311;
	kk.top = 43;
	kk.bottom = 763;

	IDirect3DSurface9* pbbSurface;
	IDirect3DSurface9* pbbTexSurface;

	g_Device->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pbbSurface);
	BBFrame->GetSurfaceLevel(0, &pbbTexSurface);

	D3DXLoadSurfaceFromSurface(pbbTexSurface, NULL, NULL, pbbSurface, NULL, &kk, D3DX_DEFAULT, 0);

	D3DXSaveSurfaceToFileW(OutputPath.c_str(), D3DXIFF_JPG, pbbTexSurface, NULL, NULL);

	pbbTexSurface->Release();
	pbbSurface->Release();*/
}

void CamHandler::LoadImg(std::string Path)
{
	IDirect3DSurface9* Surf;
	ResultFrame->GetSurfaceLevel(0, &Surf);
	D3DXLoadSurfaceFromFileA(Surf, NULL, NULL, Path.c_str(), NULL, D3DX_FILTER_NONE, 0, NULL);

	Surf->Release();
	//D3DXCreateTextureFromFileA(g_Device, Path.c_str(), &ResultFrame);
}





cv::Mat ResultFrameReset;
void CamHandler::BeginReset()
{
	if (Frame) Frame->Release(), Frame = NULL;

	if (BBFrame) BBFrame->Release(), BBFrame = NULL;

	if (ResultFrame)
	{
		D3DLOCKED_RECT Rect;
		if (ResultFrame->LockRect(0, &Rect, 0, 0) == D3D_OK) {
			cv::Mat m1(CAM_H, CAM_W, CV_8UC4, Rect.pBits, Rect.Pitch);
			m1.copyTo(ResultFrameReset);
			ResultFrame->UnlockRect(0);
		}

		ResultFrame->Release(), ResultFrame = NULL;
	}
}

void CamHandler::AfterReset()
{
	if (FAILED(g_Device->CreateTexture(CAM_W, CAM_H, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &Frame, NULL)))
		throw std::runtime_error("Failed to create Frame");

	if (FAILED(g_Device->CreateTexture(CAM_W, CAM_H, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &ResultFrame, NULL)))
		throw std::runtime_error("ResultFrame");

	if (FAILED(g_Device->CreateTexture(CAM_W, CAM_H, 1, D3DUSAGE_DYNAMIC, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &BBFrame, NULL)))
		throw std::runtime_error("BBFrame");

	if (ResultFrame)
	{
		D3DLOCKED_RECT Rect;
		if (ResultFrame->LockRect(0, &Rect, 0, 0) == D3D_OK) {
			cv::Mat m1(CAM_H, CAM_W, CV_8UC4, Rect.pBits, Rect.Pitch);
			ResultFrameReset.copyTo(m1);
			ResultFrame->UnlockRect(0);
		}
	}
}
