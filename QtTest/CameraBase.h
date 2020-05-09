#pragma once

#include <QObject>
#include <QSize>

#include <queue>

#include "opencv2/core/core.hpp"
#include "opencv2/core/mat.hpp"

/*
@brief
相机封装基类 V100Pro通用相机接口 包含通用接口与常用参数
*/
class camerabase : public QObject
{
	Q_OBJECT

public:
	/*
	@brief
	相机打开状态
	@param[1] OpenSuccess	打开成功		0
	@param[2] NoCameras		无可用相机	1
	@param[3] OpenFailed	打开失败		2
	*/
	enum OpenStatus
	{
		OpenSuccess = 0,
		NoCameras,
		OpenFailed,
	};

	/*
	@brief
	相机关闭状态
	@param[1] CloseSuccess	关闭成功		0
	@param[2] CloseFailed	关闭失败		1
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
	相机开启通用接口
	@note
	纯虚函数 必须手动复写
	@return
	OpenStatus
	*/
	virtual int openCamera() = 0;
	/*
	@brief
	从相机获取一帧的通用接口
	@param[1] frame 帧的内存需要由调用者自行申请与释放 这里只做数据复制 cv::Mat*
	@note
	V100Pro默认使用opencv cpp风格的帧格式Mat作为默认的帧格式
	纯虚函数 必须手动复写
	*/
	virtual bool get_one_frame(cv::Mat* frame) = 0;
	/*
	@brief
	相机关闭通用接口
	@note
	纯虚函数 必须手动复写
	@return
	CloseStatus
	*/
	virtual int closeCamera() = 0;

	/*
	@brief
	获取图像长宽
	*/
	virtual void get_frame_wh(QSize& s){ s = QSize(m_frame_width, m_frame_height); }
	virtual void get_frame_wh(int& w, int& h) { w = m_frame_width; h = m_frame_height; }

	/*
	@brief
	获取图像面积
	@note
	图像面积"通常"与需要申请的内存空间大小相等
	*/
	virtual int get_frame_area(){ return m_frame_area; }

	/*
	@brief
	获取相机是否已连接
	*/
	virtual bool is_camera_connected(){ return m_is_connected; }

	/*
	@brief
	获取当前图像队列大小
	*/
	virtual int get_frame_queue_size(){ return m_framedata_queue.size(); }

	/*
	@brief
	获取当前fps
	*/
	virtual double get_framerate() { return m_framerate; }

protected:
	int m_frame_width;		// 图像宽度
	int m_frame_height;		// 图像高度
	int m_frame_area;		// 图像面积

	bool m_is_connected;	// 连接状态
	double m_framerate;		// fps

	std::queue<uchar*> m_framedata_queue;	// 帧数据队列

signals:
	/*
	@brief
	相机插入通用信号
	*/
	void camera_plugin();
	/*
	@brief
	相机拔出通用信号
	*/
	void camera_plugout();
};
