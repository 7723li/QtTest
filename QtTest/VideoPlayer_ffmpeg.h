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
// ��Ƶ����(��λ:����)
typedef int64_t videoduration_ms;

/*
@brief	ͨ����Ƶ����ͼ���ſؼ����
@author lzx
@date	20200501
@note
��Ҫ��
VideoPlayer_ffmpeg_Controler_kit	(�����������������ؼ� parent : VideoPlayer_ffmpeg_Controler)
VideoPlayer_ffmpeg_Controler		(������������� parent : VideoPlayer_ffmpeg)

VideoPlayer_ffmpeg_FrameProcesser	(ͼ�����߳� ���š���ʽת�����������������ʾ parent : VideoPlayer_ffmpeg)
VideoPlayer_ffmpeg_FrameCollector	(��Ƶ�����߳� ��ȡ��Ƶ���ԡ���ȡÿһ֡ parent : VideoPlayer_ffmpeg)
VideoPlayer_ffmpeg_kit				(�����������ؼ� parent : VideoPlayer_ffmpeg)
VideoPlayer_ffmpeg					(���������� parent : �ⲿ����)
*/
#define VideoPlayer_ffmpeg_HELPER

/*
@brief
��������
@note
�����źŴ���
VideoPlayer_ffmpeg_FrameCollector(��Ƶ�����߳�)		VideoPlayer_ffmpeg_Controler(������� ���١���ͣ�����š�������)
|													[�����ؼ� VideoPlayer_ffmpeg_Controler_kit]
|													|
|													|
VideoPlayer_ffmpeg(������)----------------------------
[�����ؼ� VideoPlayer_ffmpeg_kit]
*/
#define VideoPlayer_ffmpeg_PROCESS_ORDER

class VideoPlayer_ffmpeg;

/*
@brief
�����ݴ����� ���ڻ�ȡ��Ƶ �Լ� ����Ƶ��ȡͼ�� ��ת��ͼ���ʽ
@note
1��ʹ��ffmpeg����Ƶ�ָ����Ƶ��Ƶ�� ����֡����ͨ���źŷ����ȥ
2��important!!! ÿ�β������ �� �رղ��Ŵ��� ��������ζ�ű��̱߳��ս�!!!
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
	�����������Ƶ·�� ��ʼ��ffmpeg���л���
	@param[1] video_path	�������Ƶ·��			QString
	@param[2] show_size		ͼ�����ŵ���ʾ�Ĵ�С		QSize
	@param[3] playspeed		�����ٶ� Ĭ��Ϊ1			int
	@return ����ֵ int
	<0		��ʼ��ʧ��
	other	��Ƶ��ʱ��(��λ:����)
	*/
	videoduration_ms init(const QString& video_path);
	void set_showsize(QSize show_size);
	void set_playspeed(int playspeed);

public slots:
	/*
	@brief
	����
	@param[1] begin		��ʼλ�� Ĭ��Ϊ��ͷ(0)��ʼ		int
	@param[2] finish	����λ�� Ĭ��Ϊ�����Ϊֹ(-1)		int
	*/
	void play(int begin = 0, int finish = -1);
	void pause();
	void stop();
	void leap(int msec);

	/*
	@brief
	�Ƿ����ڲ���
	@note
	�ж����� : �߳��������� �� ����״̬Ϊ��̬
	*/
	bool playing();
	/*
	@brief
	�Ƿ�������ͣ
	@note
	�ж����� : �߳��������� �� ����״̬Ϊ��̬
	*/
	bool pausing();

protected:
	virtual void run();

private:
	int read_and_show();

	/*!
	@brief
	�ͷ�ʹ�õ�����Ƶ��Դ
	*/
	void relaese();

signals:
	/*
	@brief
	�ɼ���һ֡���ź�
	@param[1] image �Ѿ�ת���˸�ʽ�ʹ�С��ͼ�� ��������ʾ QImage
	*/
	void collect_one_frame(const cv::Mat image);
	/*
	@brief
	������ +1000ms ����ʱ��
	@param[1] msec	���Ž���(��λ:����)
	*/
	void playtime_changed(int msec);
	/*
	@brief
	�ɼ�ֹͣ���ź� ��ʾ��Ƶ���Ž���
	*/
	void finish_collect_frame();

private:
	AVFormatContext* m_format_context;			// ��Ƶ����
	AVCodecContext* m_video_codeccontext;		// ����������
	AVStream* m_videostream;					// ��Ƶ��
	AVCodec* m_video_codec;						// ��Ƶ������
	AVFrame* m_orifmt_frame;					// ���ڴ���Ƶ��ȡ��ԭʼ֡�ṹ
	AVFrame* m_swsfmt_frame;					// ���ڽ� ԭʼ֡�ṹ ת������ʾ��ʽ�� ֡�ṹ
	SwsContext* m_img_convert_ctx;				// ����ת��ͼ���ʽ��ת����
	uint8_t* m_frame_buffer;					// ֡������
	AVPacket* m_read_packct;					// ���ڶ�ȡ֡�����ݰ�

	QImage::Format m_image_fmt;					// ��ʾ��ͼ���ʽRGB24
	QSize m_showsize;							// ��ʾ���ڴ�С
	int m_videostream_idx;						// ��Ƶ��λ��
	int m_play_speed;							// �����ٶ�
	int m_framenum;								// ��֡��
	int m_fps;									// ��Ƶ֡��
	double m_second_timebase;					// ���ڼ���(��)����ʱ���
	/*
	@brief
	������Ƶ֡�� ������� ������ÿ֡����������Ҫ��ʱ��(��λ:ms)
	@see
	���崦�����Ϊ
	ffmpeg����һ֡AVFrame ->
	��Ƶ��ʽת��AVFrame -> 
	���ݸ�ʽת��cv::Mat -> 
	����cv::Mat -> 
	���ݸ�ʽת��QImage -> 
	�����ź�
	@note
	���ڼ����߳���һ��ѭ��֮�� �߳���Ҫ���ߵ�ʱ�䳤��
	*/
	double m_process_perframe;
	videoduration_ms m_video_msecond;				// ��Ƶ������

	bool m_is_thred_run;						// �߳��ս�λ

	/*
	@brief
	����������״̬
	@param[1] STATIC	���洦�ھ�ֹ״̬ ��ͣ��δ���Ŷ�ʹ�����״̬
	@param[2] DYNAMIC	���洦�ڶ�̬ ����ʱʹ�����״̬
	*/
	typedef enum class play_status
	{
		// @param[1] STATIC	���洦�ھ�ֹ״̬ ��ͣ��δ���Ŷ�ʹ�����״̬
		STATIC = 0,
		// @param[2] DYNAMIC	���洦�ڶ�̬ ����ʱʹ�����״̬
		DYNAMIC = !STATIC
	}
	play_status;
	play_status m_play_status;
};

/*
@brief
��Ƶ�����������ؼ�
*/
typedef struct VideoPlayer_ffmpeg_ControlPanel_kit : public QWidget
{
public:
	explicit VideoPlayer_ffmpeg_ControlPanel_kit(QLabel* p);
	~VideoPlayer_ffmpeg_ControlPanel_kit(){}

public:
	QSlider* slider;										// ���Ž�����
	QPushButton* play_or_pause_btn;							// ������ͣ��ť
	QLabel* video_playtime;									// ��Ƶ����ʱ��
	QLabel* video_sumtime;									// ��Ƶ��ʱ��
	QComboBox* play_speed_box;								// ����ѡ��

	QStackedWidget* func_button_list;						// ���ܼ��б�
	QPushButton* delete_btn;								// ɾ����Ƶ
	QPushButton* shear_btn;									// ������Ƶ

	using scrsh_pnt_list = std::map<QGraphicsEllipseItem*, QPoint>;
	scrsh_pnt_list screenshot_pnt_list;						// ��ͼ���б�
}
VideoPlayer_ffmpeg_Controler_kit;

/*
@brief
��Ƶ���������
*/
typedef struct VideoPlayer_ffmpeg_kit
{
public:
	explicit VideoPlayer_ffmpeg_kit(VideoPlayer_ffmpeg* p = nullptr);

public:
	QLabel* frame_displayer;								// ֡��ʾ��
	VideoPlayer_ffmpeg_ControlPanel_kit* controler;			// �������
}
VideoPlayer_ffmpeg_kit;

/*
@brief
��Ƶ���������� �������ݴ���������ȡ֡���ݲ�����
*/
class VideoPlayer_ffmpeg :public QWidget
{
	Q_OBJECT

public:
	typedef enum class video_or_gif
	{
		VideoStyle = 0,				// ��Ƶ��ʽ
		GifStyle_without_slider,	// �޽������Ķ�ͼ��ʽ
		GifStyle_with_slider		// ���������Ķ�ͼ��ʽ
	}
	video_or_gif;

public:
	explicit VideoPlayer_ffmpeg(QWidget* p = nullptr);
	~VideoPlayer_ffmpeg();

public slots:
	/*
	@brief
	������Ƶ·�� ���ò���ý��
	Ĭ��Ϊ��Ƶ��ʾģʽ(���������пؼ� �� ���ŷ�ΧΪ��ͷ��β)
	@param[1] video_path	��Ƶ���·�������·��									QString
	@param[2] sta			��������ʽ ��Ƶ ��ͼ(�ַ��н�������û������)				video_or_gif
	@param[3] begin			��ͼ����ר�� ��Ƶ���ŷ�Χ��ͷ(��λ:����) Ĭ�ϴ�ͷ��ʼ ��0		int
	@param[4] finish		��ͼ����ר�� ��Ƶ���ŷ�Χ��β(��λ:����) Ĭ�ϴ�ͷ��ʼ ��-1	int
	*/
	void set_media(const QString & video_path, 
		video_or_gif sta = video_or_gif::VideoStyle,
		int beginms = 0, 
		int finishms = -1);
	void set_style(video_or_gif sta = video_or_gif::VideoStyle);
	/*
	@brief
	��ͼ����ר�� ������Ƶ���ŷ�Χ(��λ:����)
	*/
	void set_range(int beginms, int endms);
	/*
	@brief
	��ͼ����ר�� ������Ƶ���ſ�ʼʱ��(��λ:����)
	*/
	void set_begin(int beingms);
	/*
	@brief
	��ͼ����ר�� ������Ƶ���Ž���ʱ��(��λ:����)
	*/
	void set_finish(int finishms);
	/*
	@brief
	���ñ���
	*/
	void set_playspeed(int playspeed);

protected:
	/*
	@brief
	�����¼�
	@note
	��ʾ�������
	*/
	void enterEvent(QEvent* event);
	/*
	@brief
	�˳��¼�
	@ntoe
	���ؿ������
	*/
	void leaveEvent(QEvent* event);
	/*
	@brief
	�����¼�
	@note
	1��ctrl + w �رղ�����
	*/
	void keyPressEvent(QKeyEvent* event);
	/*
	@brief
	�¼�������
	@note
	1���ڲ�����QSlider������� ʵ�ֲ��Ž������洦�ɵ�Ĺ���
	@see
	ʹ���¼������� ��Ҫ�ڹ��캯������son->installEventFilter(par)���� ��ע�������
	*/
	bool eventFilter(QObject* watched, QEvent* event);

private:
	void clear_videodisplayer();

private slots:
	/*
	@brief
	��ʾһ֡
	*/
	void slot_show_one_frame(const cv::Mat image);
	/*
	@brief
	��������ת(��λ:ms)
	*/
	void slot_playtime_changed(int msec);

	/*
	@brief
	������ͣ
	*/
	void slot_play_or_pause();
	/*
	@brief
	�������������
	*/
	void slot_slider_pressed();
	void slot_slider_triggered(int action);
	void slot_slider_released();

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
	int m_slider_pressed_value;

	bool m_controller_always_hide;
};
