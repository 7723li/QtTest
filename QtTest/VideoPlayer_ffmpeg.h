#pragma once

#include <windows.h>

#include <QLoggingCategory>
#include <QFile>
#include <QFileInfo>
#include <QThread>
#include <QImage>
#include <QPixmap>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QComboBox>
#include <QGraphicsEllipseItem>
#include <QStackedWidget>
#include <QMouseEvent>

extern "C"
{
#include "externalFile/ffmpeg/include/libavcodec/avcodec.h"
#include "externalFile/ffmpeg/include/libavformat/avformat.h"
#include "externalFile/ffmpeg/include/libswscale/swscale.h"
#include "externalFile/ffmpeg/include/libavdevice/avdevice.h"
#include "externalFile/ffmpeg/include/libavutil/pixfmt.h"
}

/*
@brief	通用视频、动图播放控件简介
@author lzx
@date	20200501
@note
主要类
VideoPlayer_ffmpeg_Controler_kit	(播放器控制面板包含控件 parent : VideoPlayer_ffmpeg_Controler)
VideoPlayer_ffmpeg_Controler		(播放器控制面板 parent : VideoPlayer_ffmpeg)

VideoPlayer_ffmpeg_FrameProcesser	(图像处理线程 缩放、格式转换、发往主体进行显示 parent : VideoPlayer_ffmpeg)
VideoPlayer_ffmpeg_FrameCollector	(视频解析线程 获取视频属性、获取每一帧 parent : VideoPlayer_ffmpeg)
VideoPlayer_ffmpeg_kit				(播放器包含控件 parent : VideoPlayer_ffmpeg)
VideoPlayer_ffmpeg					(播放器主体 parent : 外部主体)
*/
#define VideoPlayer_ffmpeg_HELPER

/*
@brief
处理流程
@note
播放信号传递
VideoPlayer_ffmpeg_FrameCollector(视频解析线程)		VideoPlayer_ffmpeg_Controler(控制面板 倍速、暂停、播放、进度条)
|													[包含控件 VideoPlayer_ffmpeg_Controler_kit]
|													|
VideoPlayer_ffmpeg_FrameProcesser(图像处理线程)		|
|													|
|													|
VideoPlayer_ffmpeg(播放器)----------------------------
[包含控件 VideoPlayer_ffmpeg_kit]
*/
#define VideoPlayer_ffmpeg_PROCESS_ORDER

class VideoPlayer_ffmpeg;
class VideoPlayer_ffmpeg_ControlPanel;

/*
@brief
视频控制面板包含控件
*/
typedef struct VideoPlayer_ffmpeg_ControlPanel_kit
{
public:
	explicit VideoPlayer_ffmpeg_ControlPanel_kit(VideoPlayer_ffmpeg_ControlPanel* p);
	~VideoPlayer_ffmpeg_ControlPanel_kit(){}

public:
	QSlider* slider;										// 播放进度条
	QPushButton* play_or_pause_btn;							// 播放暂停按钮
	QLabel* video_playtime;									// 视频播放时间
	QLabel* video_sumtime;									// 视频总时长
	QComboBox* play_speed_box;								// 倍速选择

	QPushButton* delete_btn;								// 删除视频
	QPushButton* shear_btn;									// 剪切视频
	QStackedWidget* func_button_list;						// 功能键列表

	using scrsh_pnt_list = std::map<QGraphicsEllipseItem*, QPoint>;
	scrsh_pnt_list screenshot_pnt_list;						// 截图点列表
}
VideoPlayer_ffmpeg_Controler_ket;

/*
@brief
视频控制面板
*/
class VideoPlayer_ffmpeg_ControlPanel : public QSlider
{
public:
	explicit VideoPlayer_ffmpeg_ControlPanel(VideoPlayer_ffmpeg* p);
	~VideoPlayer_ffmpeg_ControlPanel(){}

private:
	VideoPlayer_ffmpeg_ControlPanel_kit* m_control_kit;
};

/*
@brief

*/
class VideoPlayer_ffmpeg_FrameProcesser : public QThread
{
public:
	explicit VideoPlayer_ffmpeg_FrameProcesser(VideoPlayer_ffmpeg* p);
	~VideoPlayer_ffmpeg_FrameProcesser(){}
};

/*
@brief
流数据处理器 用于获取视频 以及 从视频获取图像
@note
使用ffmpeg将视频分割成视频音频流 并将帧数据通过信号发射出去
*/
class VideoPlayer_ffmpeg_FrameCollector : public QThread
{
	Q_OBJECT

public:
	explicit VideoPlayer_ffmpeg_FrameCollector(VideoPlayer_ffmpeg* p = nullptr);
	~VideoPlayer_ffmpeg_FrameCollector(){}

	void set_videoname(const QString& videoname);
	void set_playspeed(int speed);
	void set_show_size(QSize size);

protected:
	virtual void run() override;

signals:
	void collect_one_frame(const QPixmap& pixmap, int prog);
	void finish_collect_frame();

private:
	QString m_videoname;
	int m_playspeed;
	QSize m_showsize;
};

/*
@brief
视频播放器组件
*/
typedef struct VideoPlayer_ffmpeg_kit
{
public:
	explicit VideoPlayer_ffmpeg_kit(VideoPlayer_ffmpeg* p = nullptr);

public:
	QLabel* frame_displayer;								// 帧显示器
	VideoPlayer_ffmpeg_ControlPanel* controler;				// 控制面板
}
VideoPlayer_ffmpeg_kit;

/*
@brief
视频播放器主体 从流数据处理器处获取帧数据并播放
*/
class VideoPlayer_ffmpeg :public QWidget
{
	Q_OBJECT

public:
	explicit VideoPlayer_ffmpeg(QWidget* p = nullptr);
	~VideoPlayer_ffmpeg();

public:
	void play(const QString & video_name);

protected:
	void enterEvent(QEvent *event);
	void 

private:
	void change_play_time();

private slots:
	void slot_show_one_frame(const QPixmap& pixmap, int prog);

	void slot_finish_collect_frame();

	void slot_play_or_pause();

	void slot_video_leap();

	void slot_change_playspeed();

	void slot_delete_video();

	void slot_shear_video();

signals:
	void play_finish();

private:
	VideoPlayer_ffmpeg_FrameCollector* m_collector;
	
	VideoPlayer_ffmpeg_kit* m_VideoPlayer_ffmpeg_kit;

	QImage m_image;
};
