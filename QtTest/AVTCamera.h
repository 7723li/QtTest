#pragma once

#include <queue>
#include <QObject>
#include <QLoggingCategory>
#include <QMutex>
#include <QMetaType>

#include "AVTCamera/include/VimbaCPP/Include/VimbaCPP.h"

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"

#include "CameraBase.h"

using namespace AVT;
using namespace AVT::VmbAPI;

class CameraObserver : public QObject, public ICameraListObserver
{
	Q_OBJECT

public:
	virtual void CameraListChanged(CameraPtr pCamera, UpdateTriggerType reason)
	{
		if (UpdateTriggerPluggedIn == reason ||
			UpdateTriggerPluggedOut == reason)
		{
			emit AVT_CameraListChanged(reason);
		}
	}

signals:
	void AVT_CameraListChanged(int reasion);
};

class FrameObserver : public QObject, public IFrameObserver
{
	Q_OBJECT

public:
	FrameObserver(CameraPtr camera) :
		IFrameObserver(camera)
	{

	}

	virtual void FrameReceived(const FramePtr newframe)
	{
		bool is_inqueue = true;
		VmbFrameStatusType eReceiveStatus;

		if (VmbErrorSuccess == newframe->GetReceiveStatus(eReceiveStatus))
		{
			m_mutex.lock();
			m_frame_queue.push_back(newframe);
			emit obsr_get_new_frame();
			m_mutex.unlock();
			is_inqueue = false;
		}

		if (is_inqueue)
		{
			m_pCamera->QueueFrame(newframe);
		}
	}

	FramePtr get_frame_queue(std::list<FramePtr>& getter)
	{
		m_mutex.lock();
		FramePtr res;
		if (!m_frame_queue.empty())
		{
			std::swap(m_frame_queue, getter);
		}
		m_mutex.unlock();
		return res;
	}

	void clear_frame_queue()
	{
		m_mutex.lock();
		std::list<FramePtr> empty;
		std::swap(m_frame_queue, empty);
		m_mutex.unlock();
	}

signals:
	void obsr_get_new_frame();

private:
	std::list<FramePtr> m_frame_queue;
	QMutex m_mutex;
};

class AVTCamera : public camerabase
{
	Q_OBJECT

public:
	typedef enum OpenStatus
	{
		OpenSucceed = 0,
		NoCameras,
		OpenFailed
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

private slots:
	void slot_camera_list_changed(int reason);
	void slot_get_new_frame();

private:
	VimbaSystem & m_vimba_system;

	ICameraListObserverPtr m_CameraObserver;
	IFrameObserverPtr m_FrameObserver;

	CameraPtrVector m_avaliable_camera_list;
	CameraPtr m_using_camera;

	VmbInt64_t m_frame_width;
	VmbInt64_t m_frame_height;
	VmbInt64_t m_frame_pixelformat;

	std::list<FramePtr> m_frame_queue;
	QMutex m_frame_mutex;
};