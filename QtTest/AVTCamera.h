#pragma once

#include <QObject>
#include <QLoggingCategory>

#include "AVTCamera/include/VimbaCPP/Include/VimbaCPP.h"

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"

#include "CameraBase.h"

using namespace AVT;
using namespace AVT::VmbAPI;

class AVTCamera : public camerabase
{
	Q_OBJECT

public:
	typedef enum OpenStatus
	{
		OpenSucceed = 0,
		NoCameras,
		OpenFailed,
		AlreadyRunning,
	}
	OpenStatus;

	typedef enum CloseStatus
	{
		CloseSucceed = 0,
		CloseFailed,
	}
	CloseStatus;

public:
	explicit AVTCamera();
	virtual ~AVTCamera();

	virtual int openCamera() override;
	virtual void getOneFrame(cv::Mat* frame) override;
	virtual int closeCamera() override;

private:
	VimbaSystem & m_vimba_system;

	CameraPtrVector m_avaliable_camera_list;
	CameraPtr m_using_camera;

	FramePtrVector m_frame_cache;
};