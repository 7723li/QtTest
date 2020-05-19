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
	other	��Ƶ��ʱ��(��λ:��)
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
	�ͷ�ʹ�õ�����Ƶ��Դ
	*/
	void free_src();

signals:
	/*
	@brief
	�ɼ���һ֡���ź�
	@param[1] image �Ѿ�ת���˸�ʽ�ʹ�С��ͼ�� ��������ʾ QImage
	@param[2] prog	���Ž���(��λ:��)
	*/
	void collect_one_frame(const cv::Mat image);
	/*
	@brief
	������ +1s ����ʱ��
	*/
	void playtime_changed(int sec);
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
	AVFrame* m_oriframe;						// ���ڴ���Ƶ��ȡ��ԭʼ֡�ṹ
	AVFrame* m_swsframe;						// ���ڽ� ԭʼ֡�ṹ ת������ʾ��ʽ�� ֡�ṹ
	SwsContext* m_img_convert_ctx;				// ����ת��ͼ���ʽ��ת����
	uint8_t* m_frame_buffer;					// ֡������
	AVPacket* m_read_packct;					// ���ڶ�ȡ֡�����ݰ�

	QImage::Format m_image_fmt;					// ��ʾ��ͼ���ʽRGB24
	QSize m_showsize;							// ��ʾ���ڴ�С
	int m_videostream_idx;						// ��Ƶ��λ��
	int m_play_speed;							// �����ٶ�
	int m_framenum;								// ��֡��
	int m_fps;									// ��Ƶ֡��
	int m_video_second;							// ��Ƶ����

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
		VideoStyle = 0,			// ��Ƶ��ʽ
		GifStyle				// ��ͼ��ʽ
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
	������ͣ
	*/
	void slot_play_or_pause();
	/*
	@brief
	�����������ת
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
