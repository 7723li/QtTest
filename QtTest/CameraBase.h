#pragma once

#include <QObject>
#include "opencv2/core/core.hpp"

class camerabase : public QObject
{
public:
	enum OpenStatus
	{
		OpenSuccess = 0,
		NotFound,
		FoundButOpenFailed,
		AlraedyRunning,
	};

	enum CloseStatus
	{
		CloseSuccess = 0,
		CloseFailed,
	};

public:
	explicit camerabase(){}
	virtual ~camerabase(){}

public:
	virtual int openCamera() = 0;
	virtual void getOneFrame(cv::Mat* frame) = 0;
	virtual int closeCamera() = 0;
};
