#pragma once

#include <QLoggingCategory>
#include <QMetaType>
#include <QTimer>

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
Vimba������۲��� ���ڼ������β�
*/
class CameraObserver : public QObject, public ICameraListObserver
{
	Q_OBJECT

public:
	/*
	@brief
	����β�ص����� �̳���ICameraListObserver
	*/
	virtual void CameraListChanged(CameraPtr pCamera, UpdateTriggerType reason) override;

signals:
	/*
	@brief
	����֪ͨ����β��¼��������ź�
	@param[1] reason �λ�� UpdateTriggerType
	*/
	void obsr_cameralist_changed(int reason);
};

/*
@brief
Vimba��֡�ص��۲��� ʵ������ΪVimbaϵͳ�Ļص�����FrameReceived
*/
class FrameObserver : public QObject, public IFrameObserver
{
	Q_OBJECT

public:
	FrameObserver(CameraPtr camera);

	/*
	@brief
	֡�ص����� �̳���IFrameObserver
	*/
	virtual void FrameReceived(const FramePtr newframe) override;

signals:
	void obsr_get_new_frame(FramePtr);		// ����֪ͨ����»�������ź�
};

/*
@brief
AVT������� �̳����������camerabase��ʵ�ֽӿ�
*/
class AVTCamera : public camerabase
{
	Q_OBJECT

public:
	explicit AVTCamera();
	virtual ~AVTCamera();

	virtual int openCamera();
	virtual bool get_one_frame(cv::Mat*);
	virtual int closeCamera();

	virtual double get_framerate() override;
	virtual bool is_camera_connected() override;

private slots:
	/*
	@brief
	������۲��ڳ��յ�����β��ź� �ж��ǰλ��ǲ� Ȼ�����Ӧ�źŸ�¼�ƽ���(PageVideoRecord)
	@param[1] reason ����ǰλ��ǲ� AVT::VmbAPI::UpdateTriggerType
	*/
	void slot_obsr_camera_list_changed(int reason);
	/*
	@brief
	��֡�۲��ߴ�������֡�����ź� ����֡������ͨ�ø�ʽ(uchar*) Ȼ��洢�����ݶ��� �ȴ�¼�ƽ���(PageVideoRecord)��ȡ
	@param[1] frame �ص�������ȡ��֡ FramePtr
	*/
	void slot_obsr_get_new_frame(FramePtr frame);

private:
	VimbaSystem & m_vimba_system;				// Vimbaϵͳ

	ICameraListObserverPtr m_CameraObserver;	// �β��¼��۲���
	IFrameObserverPtr m_FrameObserver;			// ֡�ص��۲���

	CameraPtrVector m_avaliable_camera_list;	// ����б�
	CameraPtr m_using_camera;					// ��ǰʹ���е���� Ĭ��Ϊ����б��һ��

	VmbInt64_t m_frame_pixelformat;				// ͼ���ʽ

	std::mutex m_mutex;							// �۲��߻ص����� �� ¼�ƽ�������õ����ݶ��� ��Ҫ������ֵ��ó�ͻ
};