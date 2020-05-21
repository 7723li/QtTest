#pragma once

/*
@author : lzx
@date	: 20200428
@brief
录制界面
@property
图像获取子线程(VideoCapture)
录制界面工具箱(PageVideoCollect_kit)
录制界面本体(PageVideoRecord)
*/

#include <ctime>

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTime>
#include <QTimer>
#include <QPainter>
#include <QString>
#include <QKeyEvent>
#include <QThread>
#include <QMutex>

#include "externalFile/opencv/include/opencv2/opencv.hpp"

#include "PromptBox.h"
#include "VideoListWidget.h"
#include "AVTCamera.h"
#include "VideoPlayer_ffmpeg.h"

class PageVideoRecord;

/*
@brief
录制界面的图像获取子线程
负责从相机缓冲区处获取相机解析好的帧数据
并且 保存视频
*/
class CameraCapture : public QThread
{
	Q_OBJECT

public:
	/*
	@note
	父类必须为录制界面
	*/
	CameraCapture(PageVideoRecord* p);
	~CameraCapture(){}
	
	/*
	@brief
	开始获取图像
	@param[1] c 录制界面当前正在使用的相机 camerabase*
	@param[2] m 录制界面 ->已经分配好<- 的帧空间 cv::Mat*
	@param[3] v 录制界面的VideoWriter cv::VideoWriter*
	*/
	bool begin_capture(camerabase* c, cv::Mat* m_pMat, cv::VideoWriter* v, QMutex* mt);
	/*
	@brief
	停止获取图像
	*/
	void stop_capture();

	/*
	@brief
	开始录制
	*/
	void begin_record();
	/*
	@brief
	停止录制
	*/
	void stop_record();


	/*
	@brief
	图像获取状态(相机是否正在工作)
	@note
	该状态应该必须可以影响到是否可以开始录制
	
	判断条件 : 线程正在运行 并 捕捉位为真
	*/
	bool capturing();
	/*
	@brief
	图像录制状态(是否正在录制)
	@note
	该状态应该必须可以影响到是否可以退出录制界面

	判断条件 : 线程正在运行 并 录制位为真 并 视频头为打开状态(可写入状态)
	*/
	bool recording();

private:
	/*
	@brief
	QThread线程启动后的执行部分
	*/
	virtual void run() override;

private:
	bool m_need_capture;
	bool m_need_record;

	camerabase* m_camerabase;
	cv::Mat* m_pMat;
	cv::VideoWriter* m_writer;
	QMutex* m_mutex;
};

/*
@brief
录制界面工具箱
*/
typedef struct PageVideoRecord_kit
{
public:
	explicit PageVideoRecord_kit(PageVideoRecord* p);
	~PageVideoRecord_kit(){}

public:
	QLabel* frame_displayer;			// 帧显示器
	QLabel* bed_num_label;				// 床号
	QLabel* patient_name_label;			// 姓名
	QLabel* video_time_display;			// 视频时长显示
	QPushButton* record_btn;			// (停止)录制按钮
	QPushButton* exit_btn;				// 结束检查按钮

	QWidget* video_list_background;		// 视频存放列表的纯白背景板
	VideoListWidget* video_list;		// 视频存放列表

	VideoPlayer_ffmpeg* videoplayer;	// 视频播放器
}
PageVideoRecord_kit;

/*
@brief
录制界面本体
*/
class PageVideoRecord : public QWidget
{
	Q_OBJECT

public:
	explicit PageVideoRecord(QWidget* parent);
	~PageVideoRecord(){}

public slots:
	/*
	@brief
	外部进入录制界面接口
	@param[1] examid 病例id QString
	*/
	void enter_PageVideoRecord(const QString & examid);
	/*
	@brief
	外部退出录制界面接口
	*/
	void exit_PageVideoRecord();

protected:
	void keyPressEvent(QKeyEvent* e);

private:
	void clear_videodisplay();								// 清理图像显示区域
	
	void load_old_vidthumb();								// 加载之前的录像缩略图 父视频
	void put_video_vidthumb(const QString & video_path, const QString & icon_path);	// 放置一个视频缩略图
	void clear_all_vidthumb();								// 清理录像缩略图列表

	void show_camera_openstatus(int openstatus);			// 提示相机打开状态
	void show_camera_closestatus(int closestatus);			// 提示相机关闭状态
	
	void begin_capture();									// 开始捕捉影像(开启子线程)
	void stop_capture();									// 停止捕捉影像(改变状态 关闭子线程)

	void begin_collect_fps();								// 开始收集fps
	void stop_collect_fps();								// 停止收集fps

	void begin_show_frame();								// 开始显示图像(启动定时器 运行间隔根据帧率决定)
	void stop_show_frame();									// 停止显示图像(关闭定时器)

	void begin_record();									// 从下一轮影像捕捉循环开始录制
	void stop_record();										// 从下一个影像捕捉循环停止录制

	bool apply_record_msg();								// 创建视频头

	void begin_show_recordtime();							// 开始显示录制时间
	void stop_show_recordtime();							// 停止显示录制时间

private slots:
	void slot_show_one_frame();								// 显示图像槽函数

	void slot_begin_or_finish_record();						// 开始(结束)录制视频

	void slot_show_recordtime();							// 显示录像时长

	void slot_show_framerate();

	void slot_replay_begin(QListWidgetItem* choosen_video);	// 重播录制的视频
	void slot_replay_finish();								// 重播完毕

	/*
	@brief
	相机插入
	*/
	void slot_camera_plugin();
	/*
	@brief
	相机拔出
	*/
	void slot_camera_plugout();

signals:
	void PageVideoRecord_recordbegan();						// 录制开始信号 主界面接收
	void PageVideoRecord_recordfinish();					// 录制结束信号 主界面接收
	void PageVideoRecord_exit(const QString & examid);		// 退出录制界面时发出的信号 由主界面接收

private:
	PageVideoRecord_kit* m_PageVideoRecord_kit;

	using thumb_name = std::map<QListWidgetItem*, QString>;	// 由缩略图找到视频名称的数据结构
	thumb_name m_map_video_thumb_2_name;					// 由缩略图找到视频名称

	QString m_examid;										// 病例ID 由外部传入
	QString m_video_path;									// 录制视频的保存路径
	QString m_video_thumb_path;								// 录制视频的缩略图的保存路径
	const QString m_ready_record_hint;						// 准备录像中...

	QTimer* m_show_frame_timer;								// 用于显示一帧的定时器

	QTimer* m_record_duration_timer;						// 用于记录录像时长的定时器
	QTime m_record_duration_period;							// 录像时长 每次定时器溢出自加1

	QTimer* m_show_framerate_timer;							// 用于显示fps的定时器
	bool m_is_show_framerate;								// 是否在帧显示器上显示fps
	double m_framerate;										// 当前相机的fps

	cv::Mat m_mat;											// Mat -> QImage -> QPixmap -> QLabel
	cv::VideoWriter m_VideoWriter;							// 将Mat写入到视频
	CameraCapture* m_video_capture;							// 捕捉影像的子线程
	QMutex m_mutex;											// 子线程负责写 主线程负责读 尽量避免冲突

	camerabase* m_camerabase;								// 相机封装 通用相机接口
	AVTCamera* m_avt_camera;								// AVT相机
};
