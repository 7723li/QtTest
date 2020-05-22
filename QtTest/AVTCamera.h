#pragma once

#include <QLoggingCategory>
#include <QMetaType>
#include <QTimer>
#include <QDatetime>

#include <mutex>
#include <math.h>

#include "AVTCamera/include/VimbaCPP/Include/VimbaCPP.h"
#include "AVTCamera/include/VimbaCPP/Include/VmbTransform.h"
#include "AVTCamera/include/VimbaCPP/Include/VmbCommonTypes.h"
#include "AVTCamera/include/VimbaCPP/Include/VmbTransformTypes.h"

#include "CameraBase.h"

using namespace AVT;
using namespace AVT::VmbAPI;

/*
@brief
Vimba的相机观察者 用于检测相机拔插
*/
class CameraObserver : public QObject, public ICameraListObserver
{
	Q_OBJECT

public:
	/*
	@brief
	相机拔插回调函数 继承自ICameraListObserver
	*/
	virtual void CameraListChanged(CameraPtr pCamera, UpdateTriggerType reason) override;

signals:
	/*
	@brief
	用于通知相机拔插事件发生的信号
	@param[1] reason 拔或插 UpdateTriggerType
	*/
	void obsr_cameralist_changed(int reason);
};

/*
@brief
Vimba的帧回调观察者 实际作用为Vimba系统的回调函数FrameReceived
*/
class FrameObserver : public QObject, public IFrameObserver
{
	Q_OBJECT

public:
	FrameObserver(CameraPtr camera);

	void set_camera(CameraPtr camera) { m_pCamera = camera; }

	/*
	@brief
	帧回调函数 继承自IFrameObserver
	*/
	virtual void FrameReceived(const FramePtr newframe) override;

signals:
	void obsr_get_new_frame(FramePtr);		// 用于通知相机新货到达的信号
};

/*
@brief
AVT相机本体 继承至相机父类camerabase并实现接口
*/
class AVTCamera : public camerabase
{
	Q_OBJECT

public:
	explicit AVTCamera();
	virtual ~AVTCamera();

	virtual int openCamera();
	virtual uchar* get_one_frame();
	virtual int closeCamera();

	virtual double get_framerate() override;

private slots:
	/*
	@brief
	从相机观察期出收到相机拔插信号 判断是拔还是插 然后发射对应信号给录制界面(PageVideoRecord)
	@param[1] reason 相机是拔还是插 AVT::VmbAPI::UpdateTriggerType
	*/
	void slot_obsr_camera_list_changed(int reason);
	/*
	@brief
	从帧观察者处接收新帧到达信号 并将帧解析成通用格式(uchar*) 然后存储到数据队列 等待录制界面(PageVideoRecord)获取
	@param[1] frame 回调函数获取的帧 FramePtr
	*/
	void slot_obsr_get_new_frame(FramePtr frame);
	/*
	@brief
	由定时器一秒一计算 算出上一秒的帧率
	*/
	void slot_cnt_framerate();

private:
	VimbaSystem & m_vimba_system;				// Vimba系统

	ICameraListObserverPtr m_CameraObserver;	// 拔插事件观察者
	IFrameObserverPtr m_FrameObserver;			// 帧回调观察者

	FramePtrVector m_AVT_framebuffer;

	CameraPtrVector m_avaliable_camera_list;	// 相机列表
	CameraPtr m_using_camera;					// 当前使用中的相机 默认为相机列表第一个

	VmbInt64_t m_frame_pixelformat;				// 图像格式

	std::mutex m_mutex;							// 观察者回调函数 与 录制界面均需用到数据队列 需要避免出现调用冲突

	double m_frame_obsr_cnt;					// 回调函数调用次数
	double m_save_frame_obsr_cnt;				// 保存的 上一秒的回调函数调用次数

	QTimer* m_cnt_framerate_timer;				// 用于计算fps的定时器 fps = 当前回调函数调用次数 - 上一秒回调函数调用次数
};