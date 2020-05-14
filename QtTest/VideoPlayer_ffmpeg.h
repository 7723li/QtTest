#pragma once

#include <windows.h>

#include <QLoggingCategory>
#include <QFile>
#include <QFileInfo>
#include <QThread>
#include <QImage>
#include <QPixmap>
#include <QLabel>
#include <QPaintEvent>

extern "C"
{
#include "externalFile/ffmpeg/include/libavcodec/avcodec.h"
#include "externalFile/ffmpeg/include/libavformat/avformat.h"
#include "externalFile/ffmpeg/include/libswscale/swscale.h"
#include "externalFile/ffmpeg/include/libavdevice/avdevice.h"
#include "externalFile/ffmpeg/include/libavutil/pixfmt.h"
}

class VideoPlayer_ffmpeg;

/*
@brief
�����ݴ�����
@note
ʹ��ffmpeg����Ƶ�ָ����Ƶ��Ƶ�� ����֡����ͨ���źŷ����ȥ
*/
class VideoFrameCollector_ffmpeg : public QThread
{
	Q_OBJECT

public:
	explicit VideoFrameCollector_ffmpeg(VideoPlayer_ffmpeg* p = nullptr);
	~VideoFrameCollector_ffmpeg(){}

	void set_videoname(const QString& videoname);

protected:
	virtual void run() override;

signals:
	void collect_one_frame(const QPixmap& pixmap);
	void finish_collect_frame();

private:
	QString m_videoname;
};

/*
@brief
��Ƶ���������
*/
typedef struct VideoPlayer_ffmpeg_kit : public QWidget
{
public:
	explicit VideoPlayer_ffmpeg_kit(VideoPlayer_ffmpeg* p = nullptr);

	QLabel* frame_displayer;
}
VideoPlayer_ffmpeg_kit;

/*
@brief
��Ƶ������ �������ݴ���������ȡ֡���ݲ�����
*/
class VideoPlayer_ffmpeg :public QWidget
{
	Q_OBJECT

public:
	explicit VideoPlayer_ffmpeg(QWidget* p = nullptr);
	~VideoPlayer_ffmpeg();

public:
	void play(const QString & video_name);

private slots:
	void show_frame(const QPixmap& pixmap);
	void slot_finish_collect_frame();

signals:
	void finish_play_video();

private:
	VideoFrameCollector_ffmpeg* m_collector;
	
	VideoPlayer_ffmpeg_kit* m_VideoPlayer_ffmpeg_kit;

	QImage m_image;
};
