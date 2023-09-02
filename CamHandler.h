#pragma once

#define CAM_H  720
#define CAM_W  1280 

class CamHandler {
public:

	CamHandler() = default;
	CamHandler(int CamIdx);
	~CamHandler();
	void WatchCam();
	void UpdateCamTexture();
	const LPDIRECT3DTEXTURE9 GetCamTexture();
	const LPDIRECT3DTEXTURE9 GetResultFrameTexture();
	void SetResultFrameFromCam();
	void SaveImg(std::string User);
	void SaveImgMsk(std::string User);
	void LoadImg(std::string Path);
	void BeginReset();
	void AfterReset();
	
private:

	cv::VideoCapture Cap;
	cv::Mat CapBuffer;
	LPDIRECT3DTEXTURE9 Frame;
	LPDIRECT3DTEXTURE9 ResultFrame;
	LPDIRECT3DTEXTURE9 BBFrame;
	std::mutex FrameLock;
	std::atomic<bool> StopThread = false;
	std::atomic<bool> CameraUpdated = false;
	std::future<void> Future;
};