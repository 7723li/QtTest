#pragma once

#include <QLoggingCategory>
#include <QFile>
#include <QFileInfo>
#include <QThread>
#include <QImage>
#include <QPixmap>
#include <QLabel>

extern "C"
{
#include "externalFile/ffmpeg/include/libavcodec/avcodec.h"
#include "externalFile/ffmpeg/include/libavformat/avformat.h"
#include "externalFile/ffmpeg/include/libswscale/swscale.h"
#include "externalFile/ffmpeg/include/libavdevice/avdevice.h"
#include "externalFile/ffmpeg/include/libavutil/pixfmt.h"
}

class VideoCollector_ffmpeg : public QThread
{
	Q_OBJECT

public:
	explicit VideoCollector_ffmpeg(QWidget* p = nullptr);
	~VideoCollector_ffmpeg(){}

	void set_videoname(const QString& videoname);

protected:
	void run();

signals:
	void collect_one_frame(const QPixmap& pixmap);

private:
	QString m_videoname;
};

typedef struct VideoPlayer_ffmpeg_kit
{
public:
	explicit VideoPlayer_ffmpeg_kit(QWidget* p = nullptr);

	QLabel* video_display;
}
VideoPlayer_ffmpeg_kit;

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

private:
	VideoCollector_ffmpeg* m_collector;
	
	VideoPlayer_ffmpeg_kit* m_VideoPlayer_ffmpeg_kit;
};
