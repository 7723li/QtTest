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

#include <thread>

#include "externalFile/opencv/include/opencv2/opencv.hpp"

#include "PromptBox.h"
#include "VideoListWidget.h"
#include "AVTCamera.h"
#include "VideoPlayer_ffmpeg.h"
#include "FrameDisplayWidget.hpp"


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
	FrameDisplayWidget* frame_displayer;// 帧显示器
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
	外部切换界面入口
	@param[1] examid 病例id QString
	*/
	void init_PageVideoRecord(const QString & examid);

private:
	void clear_videodisplay();								// 清理图像显示区域
	
	void load_old_videos();
	void put_one_video_thumbnail(const QString & video_path, const QString & icon_path);

	void begin_camera_and_capture();
	void stop_camera_and_capture();

	void show_camera_openstatus(int openstatus);

	void begin_capture();
	void stop_capture();

	void begin_record();
	void stop_record();

private slots:
	void slot_get_one_frame();								// 从相机处获取一帧

	void slot_begin_or_finish_record();						// 开始(结束)录制视频
	void slot_timeout_video_duration_timer();				// 相机录制计时

	void slot_replay_begin(QListWidgetItem* choosen_video);	// 重播录制的视频
	void slot_replay_finish();							// 重播完毕

	void slot_exit();										// 退出界面

signals:
	void PageVideoRecord_exit(const QString & examid);

private:
	PageVideoRecord_kit* m_PageVideoRecord_kit;

	std::vector<QString> m_recored_videoname_list;	// 当前病例之前录制过的视频 包括最新录制完成的视频

	QString m_examid;					// 病例ID 由外部传入

	QTimer* m_get_frame_timer;			// 取帧定时器
	QTimer* m_record_duration_timer;	// 用于记录录像时长的定时器
	int m_record_duration_period;		// 录像时长定时器溢出次数 = 录像时长秒数

	cv::VideoWriter m_VideoWriter;		// 将Mat写入到视频

	camerabase* m_camerabase;			// 相机封装 通用相机接口
	AVTCamera* m_avt_camera;			// AVT相机

	bool m_show_frame;
};
