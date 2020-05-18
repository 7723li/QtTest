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
VideoPlayer_ffmpeg_FrameProcesser(ͼ�����߳�)		|
|													|
|													|
VideoPlayer_ffmpeg(������)----------------------------
[�����ؼ� VideoPlayer_ffmpeg_kit]
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
�����ݴ����� ���ڻ�ȡ��Ƶ �Լ� ����Ƶ��ȡͼ��
@note
ʹ��ffmpeg����Ƶ�ָ����Ƶ��Ƶ�� ����֡����ͨ���źŷ����ȥ
*/
class VideoPlayer_ffmpeg_FrameCollector : public QThread
{
	Q_OBJECT

public:
	explicit VideoPlayer_ffmpeg_FrameCollector(VideoPlayer_ffmpeg* p = nullptr);
	~VideoPlayer_ffmpeg_FrameCollector(){}

public:
	void play(
		AVCodecContext* vcc, 
		AVFormatContext* fc,
		AVPacket* avp,
		AVFrame* avfra,
		uint8_t* b,
		QSize ss,
		QImage::Format img_fmt,
		int playspeed);
	void pause();
	void stop();

protected:
	virtual void run() override;

private:
	void init();

signals:
	void collect_one_frame(const QPixmap& pixmap, int prog);
	void finish_collect_frame();

private:
	bool m_is_play;
	AVCodecContext* m_vcc;
	AVFormatContext* m_fc;
	AVPacket* m_avp;
	AVFrame* m_avfra;
	uint8_t* m_b;
	QSize m_ss;
	QImage::Format m_img_fmt;
	int m_playspeed;
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
	void play(const QString & video_path, 
		video_or_gif sta = video_or_gif::VideoStyle,
		int playspeed = 1);

protected:
	void enterEvent(QEvent* event);
	void leaveEvent(QEvent* event);

private:
	void init_ffmpeg();				// ��ʼ��ffmpeg��
	bool analysis_video();			// ������Ƶ	��������֡�ɼ��߳�ǰ������Դ

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

	QString m_video_path;						// ��Ƶ·��
	AVFormatContext* m_format_context;			// ��Ƶ����
	AVCodecContext* m_video_codeccontext;		// ����������
	AVCodec* m_video_codec;						// ��Ƶ������
	AVFrame* m_frame;							// ֡�ṹ
	uint8_t* m_frame_buffer;					// ֡������
	AVPacket* m_read_packct;					// ���ڶ�ȡ֡�����ݰ�

	VideoPlayer_ffmpeg_FrameCollector* m_collector;

	QImage::Format m_image_fmt;

	bool m_controller_always_hide;
};
