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
#include <QApplication>
#include <QDesktopWidget>
#include <QFileInfo>

#include "PromptBox.h"

extern "C"
{
#include "externalFile/ffmpeg/include/libavcodec/avcodec.h"
#include "externalFile/ffmpeg/include/libavformat/avformat.h"
#include "externalFile/ffmpeg/include/libswscale/swscale.h"
#include "externalFile/ffmpeg/include/libavdevice/avdevice.h"
#include "externalFile/ffmpeg/include/libavutil/pixfmt.h"
}

typedef struct SwsContext SwsContext;

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

public:
	void set_param(
		AVCodecContext* vcc, 
		AVFormatContext* fc,
		AVPacket* avp,
		AVFrame* avfra,
		AVFrame* swfra,
		SwsContext* img_con_ctx,
		uint8_t* b,
		QSize ss,
		QImage::Format img_fmt,
		int playspeed,
		int videostream_idx);
	void pause();
	void stop();

protected:
	virtual void run();

private:
	void init();

signals:
	void collect_one_frame(const QPixmap& pixmap, int prog);
	void finish_collect_frame();

private:
	AVCodecContext* m_video_codeccontext;
	AVFormatContext* m_format_context;
	AVPacket* m_read_packct;
	AVFrame* m_oriframe;
	AVFrame* m_swsframe;
	SwsContext* m_img_convert_ctx;
	uint8_t* m_frame_buffer;
	QSize m_showsize;
	QImage::Format m_img_fmt;
	int m_playspeed;
	int m_videostream_idx;

	bool m_is_run;
};

/*
@brief
视频控制面板包含控件
*/
typedef struct VideoPlayer_ffmpeg_ControlPanel_kit : public QWidget
{
public:
	explicit VideoPlayer_ffmpeg_ControlPanel_kit(QLabel* p);
	~VideoPlayer_ffmpeg_ControlPanel_kit(){}

public:
	QSlider* slider;										// 播放进度条
	QPushButton* play_or_pause_btn;							// 播放暂停按钮
	QLabel* video_playtime;									// 视频播放时间
	QLabel* video_sumtime;									// 视频总时长
	QComboBox* play_speed_box;								// 倍速选择

	QStackedWidget* func_button_list;						// 功能键列表
	QPushButton* delete_btn;								// 删除视频
	QPushButton* shear_btn;									// 剪切视频

	using scrsh_pnt_list = std::map<QGraphicsEllipseItem*, QPoint>;
	scrsh_pnt_list screenshot_pnt_list;						// 截图点列表
}
VideoPlayer_ffmpeg_Controler_kit;

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
	VideoPlayer_ffmpeg_ControlPanel_kit* controler;			// 控制面板
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
	typedef enum class video_or_gif
	{
		VideoStyle = 0,			// 视频样式
		GifStyle				// 动图样式
	}
	video_or_gif;

public:
	explicit VideoPlayer_ffmpeg(QWidget* p = nullptr);
	~VideoPlayer_ffmpeg();

public slots:
	void play(const QString & video_path, 
		video_or_gif sta = video_or_gif::VideoStyle,
		int playspeed = 1);

protected:
	void enterEvent(QEvent* event);
	void leaveEvent(QEvent* event);

private:
	void init_ffmpeg();				// 初始化ffmpeg库
	bool analysis_video();			// 解析视频	并在启动帧采集线程前分配资源

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
	VideoPlayer_ffmpeg_kit* m_VideoPlayer_ffmpeg_kit;

	QString m_video_path;						// 视频路径
	AVFormatContext* m_format_context;			// 视频属性
	AVCodecContext* m_video_codeccontext;		// 解码器属性
	AVCodec* m_video_codec;						// 视频解码器
	AVFrame* m_oriframe;						// 用于从视频读取的原始帧结构
	AVFrame* m_swsframe;						// 用于将 原始帧结构 转换成显示格式的 帧结构
	SwsContext* m_img_convert_ctx;				// 用于转换图像格式的转换器
	uint8_t* m_frame_buffer;					// 帧缓冲区
	AVPacket* m_read_packct;					// 用于读取帧的数据包
	QImage::Format m_image_fmt;					// 显示的图像格式RGB24
	int m_play_speed;							// 播放速度
	int m_videostream_idx;						// 视频流位置

	VideoPlayer_ffmpeg_FrameCollector* m_collector;

	bool m_controller_always_hide;
};
