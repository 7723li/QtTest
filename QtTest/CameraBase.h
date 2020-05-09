#pragma once

#include <QObject>
#include <QSize>

#include <queue>

#include "opencv2/core/core.hpp"
#include "opencv2/core/mat.hpp"

/*
@brief
�����װ���� V100Proͨ������ӿ� ����ͨ�ýӿ��볣�ò���
*/
class camerabase : public QObject
{
	Q_OBJECT

public:
	/*
	@brief
	�����״̬
	@param[1] OpenSuccess	�򿪳ɹ�		0
	@param[2] NoCameras		�޿������	1
	@param[3] OpenFailed	��ʧ��		2
	*/
	enum OpenStatus
	{
		OpenSuccess = 0,
		NoCameras,
		OpenFailed,
	};

	/*
	@brief
	����ر�״̬
	@param[1] CloseSuccess	�رճɹ�		0
	@param[2] CloseFailed	�ر�ʧ��		1
	*/
	enum CloseStatus
	{
		CloseSuccess = 0,
		CloseFailed,
	};

	camerabase(){}
	virtual ~camerabase(){}

public:
	/*
	@brief
	�������ͨ�ýӿ�
	@note
	���麯�� �����ֶ���д
	@return
	OpenStatus
	*/
	virtual int openCamera() = 0;
	/*
	@brief
	�������ȡһ֡��ͨ�ýӿ�
	@param[1] frame ֡���ڴ���Ҫ�ɵ����������������ͷ� ����ֻ�����ݸ��� cv::Mat*
	@note
	V100ProĬ��ʹ��opencv cpp����֡��ʽMat��ΪĬ�ϵ�֡��ʽ
	���麯�� �����ֶ���д
	*/
	virtual bool get_one_frame(cv::Mat* frame) = 0;
	/*
	@brief
	����ر�ͨ�ýӿ�
	@note
	���麯�� �����ֶ���д
	@return
	CloseStatus
	*/
	virtual int closeCamera() = 0;

	/*
	@brief
	��ȡͼ�񳤿�
	*/
	virtual void get_frame_wh(QSize& s){ s = QSize(m_frame_width, m_frame_height); }
	virtual void get_frame_wh(int& w, int& h) { w = m_frame_width; h = m_frame_height; }

	/*
	@brief
	��ȡͼ�����
	@note
	ͼ�����"ͨ��"����Ҫ������ڴ�ռ��С���
	*/
	virtual int get_frame_area(){ return m_frame_area; }

	/*
	@brief
	��ȡ����Ƿ�������
	*/
	virtual bool is_camera_connected(){ return m_is_connected; }

	/*
	@brief
	��ȡ��ǰͼ����д�С
	*/
	virtual int get_frame_queue_size(){ return m_framedata_queue.size(); }

	/*
	@brief
	��ȡ��ǰfps
	*/
	virtual double get_framerate() { return m_framerate; }

protected:
	int m_frame_width;		// ͼ����
	int m_frame_height;		// ͼ��߶�
	int m_frame_area;		// ͼ�����

	bool m_is_connected;	// ����״̬
	double m_framerate;		// fps

	std::queue<uchar*> m_framedata_queue;	// ֡���ݶ���

signals:
	/*
	@brief
	�������ͨ���ź�
	*/
	void camera_plugin();
	/*
	@brief
	����γ�ͨ���ź�
	*/
	void camera_plugout();
};
