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
#include <QKeyEvent>
#include <QTime>
#include <QPainter>
#include <QMetaType>

#include "PromptBox.h"
#include "externalFile/opencv/include/opencv2/core/core.hpp"
#include "externalFile/opencv/include/opencv2/core/mat.hpp"
#include "externalFile/opencv/include/opencv2/imgproc/imgproc.hpp"

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
|													|
VideoPlayer_ffmpeg(播放器)----------------------------
[包含控件 VideoPlayer_ffmpeg_kit]
*/
#define VideoPlayer_ffmpeg_PROCESS_ORDER

class VideoPlayer_ffmpeg;

/*
@brief
流数据处理器 用于获取视频 以及 从视频获取图像 并转换图像格式
@note
1、使用ffmpeg将视频分割成视频音频流 并将帧数据通过信号发射出去
2、important!!! 每次播放完毕 或 关闭播放窗口 都必须意味着本线程被终结!!!
*/
class VideoPlayer_ffmpeg_FrameCollector : public QThread
{
	Q_OBJECT

public:
	explicit VideoPlayer_ffmpeg_FrameCollector(VideoPlayer_ffmpeg* p = nullptr);
	~VideoPlayer_ffmpeg_FrameCollector(){}

public:
	/*
	@brief
	根据输入的视频路径 初始化ffmpeg运行环境
	@param[1] video_path	输入的视频路径			QString
	@param[2] show_size		图像缩放到显示的大小		QSize
	@param[3] playspeed		播放速度 默认为1			int
	@return 返回值 int
	<0		初始化失败
	other	视频总时长(单位:秒)
	*/
	int init(const QString& video_path,
		QSize show_size,
		int playspeed = 1);

public slots:
	void play();
	void pause();
	void stop();
	void leap(int pos);

public:
	bool playing();
	bool pausing();

protected:
	virtual void run();

private:
	/*!
	@brief
	释放使用到的视频资源
	*/
	void free_src();

signals:
	/*
	@brief
	采集到一帧的信号
	@param[1] image 已经转换了格式和大小的图像 可用于显示 QImage
	@param[2] prog	播放进度(单位:秒)
	*/
	void collect_one_frame(const cv::Mat image);
	/*
	@brief
	进度条 +1s 播放时间
	*/
	void playtime_changed(int sec);
	/*
	@brief
	采集停止的信号 表示视频播放结束
	*/
	void finish_collect_frame();

private:
	AVFormatContext* m_format_context;			// 视频属性
	AVCodecContext* m_video_codeccontext;		// 解码器属性
	AVStream* m_videostream;					// 视频流
	AVCodec* m_video_codec;						// 视频解码器
	AVFrame* m_oriframe;						// 用于从视频读取的原始帧结构
	AVFrame* m_swsframe;						// 用于将 原始帧结构 转换成显示格式的 帧结构
	SwsContext* m_img_convert_ctx;				// 用于转换图像格式的转换器
	uint8_t* m_frame_buffer;					// 帧缓冲区
	AVPacket* m_read_packct;					// 用于读取帧的数据包

	QImage::Format m_image_fmt;					// 显示的图像格式RGB24
	QSize m_showsize;							// 显示窗口大小
	int m_videostream_idx;						// 视频流位置
	int m_play_speed;							// 播放速度
	int m_framenum;								// 总帧数
	int m_fps;									// 视频帧率
	int m_video_second;							// 视频秒数

	bool m_is_thred_run;

	typedef enum class play_status
	{
		PAUSING = 0,
		PLAYING
	}
	play_status;
	play_status m_play_status;
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
	void set_media(const QString & video_path, 
		video_or_gif sta = video_or_gif::VideoStyle,
		int playspeed = 1);
	//void set_playrange(int begin, int end);

protected:
	void enterEvent(QEvent* event);
	void leaveEvent(QEvent* event);
	void keyPressEvent(QKeyEvent* event);

private:
	void clear_videodisplayer();

private slots:
	void slot_show_one_frame(const cv::Mat image);
	void slot_playtime_changed(int sec);

	/*
	@brief
	播放暂停
	*/
	void slot_play_or_pause();
	/*
	@brief
	点击进度条跳转
	*/
	void slot_video_leap();

	void slot_finish_collect_frame();

	void slot_change_playspeed();

	void slot_delete_video();

	void slot_shear_video();

	void slot_exit();

signals:
	void play_finish();

private:	
	VideoPlayer_ffmpeg_kit* m_VideoPlayer_ffmpeg_kit;

	VideoPlayer_ffmpeg_FrameCollector* m_collector;

	bool m_controller_always_hide;
};
