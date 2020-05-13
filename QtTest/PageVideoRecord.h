#pragma once

/*
@author : lzx
@date	: 20200428
@brief
录制界面
@property
录制界面工具箱(PageVideoCollect_kit)
录制界面本体(PageVideoRecord)
*/

#include <ctime>

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>
#include <QTime>
#include <QTimer>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QPaintEvent>
#include <QPainter>

#include <thread>

#include "externalFile/opencv/include/opencv2/opencv.hpp"

#include "PromptBox.h"
#include "VideoListWidget.h"
#include "AVTCamera.h"
#include "VideoPlayer_ffmpeg.h"

/*
@brief
录制界面工具箱
*/
typedef struct PageVideoRecord_kit
{
public:
	explicit PageVideoRecord_kit(QWidget* p);
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
	void enter_PageVideoRecord(const QString & examid);		// 进入录制界面
	/*
	@brief
	外部退出录制界面接口
	*/
	void exit_PageVideoRecord();							// 退出录制界面

private:
	void clear_videodisplay();								// 清理图像显示区域
	
	void load_old_videos();									// 加载之前的录像 父视频
	
	void put_video_thumb(const QString & video_path, const QString & icon_path);	// 放置一个视频缩略图

	bool open_camera();										// 打开相机
	void close_camera();									// 关闭相机

	void show_camera_openstatus(int openstatus);			// 提示相机打开状态
	
	void begin_capture();									// 开始捕捉影像(开启子线程)
	void stop_capture();									// 停止捕捉影像(改变状态 关闭子线程)
	void capture_thread();									// 子线程 从相机的缓冲区处获取新的一帧

	bool get_useful_fps(double & fps);						// 获取一个有效(>0)的帧率 用于显示和录制

	void begin_show_frame();								// 开始显示图像(启动定时器 运行间隔根据帧率决定)
	void stop_show_frame();									// 停止显示图像(关闭定时器)

	void begin_record();									// 开始录制
	void stop_record();										// 停止录制(改变状态 保存视频)

private slots:
	void slot_show_one_frame();								// 显示图像槽函数

	void slot_begin_or_finish_record();						// 开始(结束)录制视频

	void slot_timeout_video_duration_timer();				// 显示录像时长

	void slot_replay_begin(QListWidgetItem* choosen_video);	// 重播录制的视频
	void slot_replay_finish();								// 重播完毕

signals:
	void PageVideoRecord_exit(const QString & examid);		// 退出录制界面时发出的信号 由主界面接收

private:
	PageVideoRecord_kit* m_PageVideoRecord_kit;

	std::vector<QString> m_recored_videoname_list;			// 当前病例之前录制过的视频 包括最新录制完成的视频

	QString m_examid;										// 病例ID 由外部传入

	QTimer* m_show_frame_timer;								// 显示图像定时器
	QTimer* m_record_duration_timer;						// 用于记录录像时长的定时器
	int m_record_duration_period;							// 录像时长定时器溢出次数 = 录像时长秒数

	cv::Mat m_mat;											// 帧数据缓存
	cv::VideoWriter m_VideoWriter;							// 将Mat写入到视频

	camerabase* m_camerabase;								// 相机封装 通用相机接口
	AVTCamera* m_avt_camera;								// AVT相机

	bool m_is_need_capture;									// 是否需要(可以)捕捉影像
	bool m_is_capturing;									// 是否正在捕捉影像

	bool m_is_need_record;									// 是否需要(可以)录制视频
	bool m_is_recording;									// 是否正在录制视频
};
