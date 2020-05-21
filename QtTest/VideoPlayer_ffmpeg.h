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
#include <QMutex>

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

using SwsContext = struct SwsContext;
// 视频长度(单位:毫秒)
using videoduration_ms = int64_t;

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
	other	视频总时长(单位:毫秒)
	*/
	videoduration_ms init(const QString& video_path);
	void set_showsize(QSize show_size);
	/*
	@brief!
	播放倍速

	@note
	增加播放倍速 即意味着增加播放帧率
	*********************************************************
	比如
	60fps的视频2倍速播放 则意味着播放帧率应该提升到120fps
	60fps的视频0.5倍速播放 则播放帧率应为30fps
	*********************************************************
	改变播放帧率
	最直观的处理方式就是改变计算出的单帧理论处理时间
	减少或增加线程休眠时间
	*********************************************************

	@example
	提升至2倍速		->	单帧理论处理时间减半 / 2
	降低至0.5倍速	->	单帧理论处理时间加倍 / 0.5
	*/
	void set_playspeed(double playspeed);
	/*
	@brief
	设置播放范围
	*/
	void set_range(videoduration_ms begin, videoduration_ms end);

	/*
	@brief!
	查看是否播放完毕
	@note
	判断条件 : 线程没有正在运行 并 播放状态为静态
	*/
	bool is_end();
	/*
	@brief
	是否正在播放
	@note
	判断条件 : 线程不在暂停状态 并 播放状态为动态
	*/
	bool is_playing();
	/*
	@brief
	是否正在暂停
	@note
	判断条件 : 线程在暂停状态 并 播放状态为静态
	*/
	bool is_pause();

public slots:
	/*
	@brief
	播放
	@param[1] begin		开始位置 默认为从头(0)开始		int
	@param[2] finish	结束位置 默认为到最后为止(-1)		int
	*/
	void play(int begin = 0, int finish = -1);
	void pause();
	void stop();
	/*
	@brief!
	视频跳转功能
	@param[1]	视频播放时间(单位:毫秒)
	*/
	void leap(int msec);
	/*!
	@brief
	释放使用到的视频资源
	*/
	void relaese();

protected:
	virtual void run();

signals:
	/*
	@brief
	采集到一帧的信号
	@param[1] image 已经转换了格式和大小的图像 可用于显示 QImage
	*/
	void collect_one_frame(const cv::Mat image);
	/*
	@brief
	进度条 +1000ms 播放时间
	@param[1] msec	播放进度(单位:毫秒)
	*/
	void playtime_changed(int msec);
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
	AVFrame* m_orifmt_frame;					// 用于从视频读取的原始帧结构
	AVFrame* m_swsfmt_frame;					// 用于将 原始帧结构 转换成显示格式的 帧结构
	SwsContext* m_img_convert_ctx;				// 用于转换图像格式的转换器
	uint8_t* m_frame_buffer;					// 帧缓冲区
	AVPacket* m_read_packct;					// 用于读取帧的数据包

	QImage::Format m_image_fmt;					// 显示的图像格式RGB24
	QSize m_showsize;							// 显示窗口大小
	int m_videostream_idx;						// 视频流位置
	int m_framenum;								// 总帧数
	int m_fps;									// 视频帧率
	double m_second_timebase;					// 用于计算(秒)数的时间基
	/*
	@brief
	1倍速情况下
	根据视频帧率计算出的
	理论上每帧单独处理需要的时间(单位:ms)
	@attention
	只在初始化视频时进行一次计算赋值
	在运行过程中必须不可改变
	@see
	具体处理过程为
	ffmpeg解析一帧AVFrame ->
	视频格式转换AVFrame -> 
	数据格式转换cv::Mat -> 
	缩放cv::Mat -> 
	数据格式转换QImage -> 
	发送信号
	@note
	用于计算线程中一个循环之后 线程需要休眠的时间长度
	*/
	double m_process_perframe_1speed;
	/*
	@brief!
	应用于线程中的单帧处理时间
	运行过程中可改变
	*/
	double m_process_perframe;	
	videoduration_ms m_video_msecond;			// 视频毫秒数
	videoduration_ms m_video_playbegin;			// 视频播放开始位置
	videoduration_ms m_video_playend;			// 视频播放结束位置

	bool m_is_thread_run;						// 线程开关位
	bool m_is_thread_pausing;					// 线程暂停判断位
	bool m_is_init;

	/*
	@brief
	播放器播放状态
	@param[1] STATIC	界面处于静止状态 暂停、未播放都使用这个状态
	@param[2] DYNAMIC	界面处于动态 播放时使用这个状态
	*/
	typedef enum class play_status
	{
		// @param[1] STATIC	界面处于静止状态 暂停、未播放都使用这个状态
		STATIC = 0,
		// @param[2] DYNAMIC	界面处于动态 播放时使用这个状态
		DYNAMIC = !STATIC
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
	Q_OBJECT

public:
	explicit VideoPlayer_ffmpeg_ControlPanel_kit(QLabel* p);
	~VideoPlayer_ffmpeg_ControlPanel_kit(){}

public:
	QPushButton* play_or_pause_btn;							// 播放暂停按钮
	QLabel* video_playtime;									// 视频播放时间
	QLabel* video_sumtime;									// 视频总时长
	QComboBox* playspeed_box;								// 倍速选择

	QStackedWidget* func_button_list;						// 功能键列表
	QPushButton* delete_btn;								// 删除视频
	QPushButton* shear_btn;									// 剪切视频
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
	QSlider* slider;										// 播放进度条
	VideoPlayer_ffmpeg_ControlPanel_kit* controler;			// 控制面板
}
VideoPlayer_ffmpeg_kit;

/*
@brief
视频播放器主体 从流数据处理器处获取帧数据并播放
@note
默认隐藏
*/
class VideoPlayer_ffmpeg :public QWidget
{
	Q_OBJECT

public:
	/*
	@brief
	显示状态
	@param[1] VideoStyle				视频样式
	@param[2] GifStyle_without_slider	无进度条的动图样式
	@param[3] GifStyle_with_slider		带进度条的动图样式
	*/
	enum class video_or_gif
	{
		VideoStyle = 0,
		GifStyle_without_slider,
		GifStyle_with_slider,
	};

	/*
	@brief
	媒介设置状态
	@param[1] Succeed				成功
	@param[2] NoVideoMessage		没有找到视频信息
	@param[3] VideoAnalysisFailed	视频解析错误
	*/
	enum class set_media_status
	{
		Succeed = 0,
		NoVideoMessage,
		VideoAnalysisFailed
	};

public:
	explicit VideoPlayer_ffmpeg(QWidget* p = nullptr);
	~VideoPlayer_ffmpeg(){}

public slots:
	/*
	@brief
	根据视频路径 设置播放媒介
	默认为视频显示模式(即带有所有控件 并 播放范围为由头到尾)
	@param[1] video_path	视频相对路径或绝对路径							QString
	@param[2] sta			播放器样式 视频 或动图(又分有进度条和没进度条)		video_or_gif
	@param[3] begin			视频播放范围开头(单位:毫秒) 默认从头开始 即0		int
	@param[4] finish		视频播放范围结尾(单位:毫秒) 默认从头开始 即-1		int
	@return!
	返回值
	true	设置成功
	false	设置失败
	@note!
	只需要在设置视频路径的时候调用一次 设置路径后避免重复调用
	*/
	set_media_status set_media(const QString & video_path,
		video_or_gif sta = video_or_gif::VideoStyle,
		videoduration_ms beginms = 0,
		videoduration_ms endms = -1);
	void set_style(video_or_gif sta = video_or_gif::VideoStyle);
	/*
	@brief
	设置视频播放范围(单位:毫秒)
	@attention
	播放范围 与 进度条活动范围没有直接关联
	*/
	void set_playrange(videoduration_ms beginms, videoduration_ms endms);
	/*
	@brief
	设置视频播放开始时间(单位:毫秒)
	*/
	void set_playbegin(videoduration_ms beingms);
	/*
	@brief
	设置视频播放结束时间(单位:毫秒)
	*/
	void set_playend(videoduration_ms finishms);
	/*
	@brief
	设置默认播放倍速
	@attention
	视频、动图倍速默认都为1倍速
	预留接口
	正常情况下不应该被调用 
	除非之后动图需要倍速播放
	否则永远不会用到
	*/
	void set_playspeed(int playspeed);
	/*
	@brief
	gif播放专用 设置是否重复播放
	@note
	调用这个方法后将会马上开始播放视频
	*/
	void set_replay(bool replay);

protected:
	/*
	@brief
	进入事件
	@note
	显示控制面板
	*/
	void enterEvent(QEvent* event);
	/*
	@brief
	退出事件
	@ntoe
	隐藏控制面板
	*/
	void leaveEvent(QEvent* event);
	/*
	@brief
	键盘事件
	@note
	1、ctrl + w 关闭播放器
	*/
	void keyPressEvent(QKeyEvent* event);
	/*
	@brief
	事件过滤器
	@note
	1、在不重载QSlider的情况下 实现播放进度条随处可点的功能
	@see
	使用事件过滤器 需要在构造函数调用son->installEventFilter(par)功能 来注册过滤器
	*/
	bool eventFilter(QObject* watched, QEvent* event);

private:
	void clear_videodisplayer();
	void init_playspeed_box();

private slots:
	/*
	@brief
	显示一帧
	*/
	void slot_show_one_frame(const cv::Mat image);
	/*
	@brief
	进度条跳转(单位:ms)
	*/
	void slot_playtime_changed(int msec);

	/*
	@brief
	播放暂停
	*/
	void slot_play_or_pause();
	/*
	@brief
	点击进度条滑块
	*/
	void slot_slider_triggered(int action);

	void slot_finish_collect_frame();

	void slot_playspeed_changed(int box_idx);

	void slot_delete_video();

	void slot_shear_video();

	void slot_exit();

signals:
	void play_finish();

	void slider_motivated(int slideraction);

private:	
	VideoPlayer_ffmpeg_kit* m_VideoPlayer_ffmpeg_kit;

	VideoPlayer_ffmpeg_FrameCollector* m_collector;

	QString m_abs_videopath;

	bool m_video_exist;
	bool m_controller_always_hide;
	bool m_replay_after_finish;

	const std::vector<double> m_playspeed_choosen;	// 可选的播放倍速
	const int m_playspeed_1_index;					// 1倍速的索引位置

	videoduration_ms m_playbeginms;
	videoduration_ms m_playendms;
};
