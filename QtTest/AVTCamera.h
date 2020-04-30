#pragma once

#include <QObject>
#include <QLoggingCategory>

#include "AVTCamera/include/VimbaCPP/Include/VimbaCPP.h"

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace AVT;
using namespace AVT::VmbAPI;

class camerabase
{
public:
	camerabase(){}
	virtual ~camerabase(){}

};

class AVTCamera : public QObject, public camerabase
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
	~AVTCamera();

	int openCamera();
	int closeCamera();
	void CopyCurrentFrame(cv::Mat*);

	//virtual int test() override;

private:
	VimbaSystem & m_vimba_system;

	CameraPtrVector m_avaliable_camera_list;
	CameraPtr m_using_camera;

	FramePtrVector m_frame_cache;
};