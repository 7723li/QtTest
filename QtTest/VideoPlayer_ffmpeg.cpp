#include "VideoPlayer_ffmpeg.h"

VideoPlayer_ffmpeg_ControlPanel_kit::VideoPlayer_ffmpeg_ControlPanel_kit(VideoPlayer_ffmpeg_ControlPanel* p)
{
	slider = new QSlider(p);
	slider->setTickPosition(QSlider::NoTicks);
	slider->setOrientation(Qt::Horizontal);
	slider->setGeometry(0, 0, p->width(), 10);

	play_or_pause_btn = new QPushButton(p);
	play_or_pause_btn->setGeometry(60, 50, 50, 50);
	play_or_pause_btn->setText("popop");

	video_sumtime = new QLabel(p);
	video_sumtime->setGeometry(120, 65, 100, 20);
	video_sumtime->setText("00:00:00");

	video_playtime = new QLabel(p);
	video_playtime->setGeometry(250, 65, 100, 20);
	video_playtime->setText("00:00:00");

	play_speed_box = new QComboBox(p);
	play_speed_box->setGeometry(400, 60, 40, 40);

	delete_btn = new QPushButton(p);
	shear_btn = new QPushButton(p);

	func_button_list = new QStackedWidget(p);
	func_button_list->addWidget(delete_btn);
	func_button_list->addWidget(shear_btn);

	func_button_list->setGeometry(450, 60, 40, 40);
}






VideoPlayer_ffmpeg_ControlPanel::VideoPlayer_ffmpeg_ControlPanel(VideoPlayer_ffmpeg* p):
QSlider(p)
{
	m_control_kit = new VideoPlayer_ffmpeg_ControlPanel_kit(this);
}




VideoPlayer_ffmpeg_FrameProcesser::VideoPlayer_ffmpeg_FrameProcesser(VideoPlayer_ffmpeg* p)
{

}




VideoPlayer_ffmpeg_FrameCollector::VideoPlayer_ffmpeg_FrameCollector(VideoPlayer_ffmpeg* p) : 
QThread(p)
{
	m_playspeed = 1;
}
void VideoPlayer_ffmpeg_FrameCollector::set_videoname(const QString& videoname)
{
	QFileInfo to_abs_filepath(videoname);		// ��ȫ��� ���۴���������ʲô������Ϊ����·��
	m_videoname = to_abs_filepath.absoluteFilePath();
}
void VideoPlayer_ffmpeg_FrameCollector::set_playspeed(int speed)
{
	m_playspeed = speed;
}
void VideoPlayer_ffmpeg_FrameCollector::set_show_size(QSize size)
{
	m_showsize = size;
}
void VideoPlayer_ffmpeg_FrameCollector::run()
{
	av_register_all();	// ��ʼ��ffmpeg����

	// ��ʽ���� �ᴩ����ffmpeg�������� ��ž���һ����Ƶ�ļ������԰�
	AVFormatContext* format_context = avformat_alloc_context();

	// �ж���Ƶ�ڲ��� ��һ����ʽ���� ��ʵ���Ǵ�һ����Ƶ ���Ҽ�"����"
	const char* file_name = m_videoname.toStdString().c_str();
	if (!QFile::exists(m_videoname) || avformat_open_input(&format_context, m_videoname.toStdString().c_str(), nullptr, nullptr) < 0)
	{
		return;
	}

	// ���� ��ʽ���� Ѱ����Ƶ������Ƶ��֮�����Ϣ(TopV��Ŀ����Ƶһ�㶼ֻ����Ƶ��)
	if (avformat_find_stream_info(format_context, nullptr) < 0)
	{
		return;
	}

	// Ѱ����Ƶ����λ��
	int video_stream_index = -1;
	for (int i = 0; i < format_context->nb_streams; ++i)
	{
		if (format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			video_stream_index = i;
			break;
		}
	}
	if (-1 == video_stream_index)
	{
		return;
	}

	// ��Ƶ���������� �������� �����ҵ�����Ƶ���Ľ���������
	AVCodecContext* video_codeccontext = format_context->streams[video_stream_index]->codec;

	// ffmpeg�ṩ��һϵ�еĽ����� ������Ǹ���id��Ҫ�õ��Ǹ�������
	AVCodec* video_codec = avcodec_find_decoder(video_codeccontext->codec_id);
	if (nullptr == video_codec)
	{
		return;
	}

	// �򿪽�����
	if (avcodec_open2(video_codeccontext, video_codec, nullptr) < 0)
	{
		return;
	}

	// ��������֡�ṹ һ����Ƶ֡��Ĭ�ϸ�ʽ(һ��ΪYUV420) һ��ת��ͼ��ĸ�ʽ(Ĭ��ΪRGB)
	AVFrame* frame = av_frame_alloc();
	AVFrame* frameRGB = av_frame_alloc();

	// one frame scale to another ��Ҫ��ת������
	struct SwsContext* img_convert_ctx = sws_getContext(
		video_codeccontext->width,
		video_codeccontext->height,
		video_codeccontext->pix_fmt,
		video_codeccontext->width,
		video_codeccontext->height,
		AV_PIX_FMT_GRAY8,
		SWS_BICUBIC,
		NULL,
		NULL,
		NULL
		);

	// (ת���ʽ)һ֡��Ҫ���ֽ��� ȡ���� ��ʽ��ͼ���С
	int num_bytes = avpicture_get_size(AV_PIX_FMT_GRAY8, video_codeccontext->width, video_codeccontext->height);
	

	// ����(ת���ʽ)һ֡��Ҫ���ֽ��� ��֡����һ�������� ��װ��һ֡������
	uint8_t* out_buffer = (uint8_t*)av_malloc(num_bytes * sizeof(uint8_t));
	avpicture_fill((AVPicture*)frameRGB, out_buffer, AV_PIX_FMT_GRAY8, video_codeccontext->width, video_codeccontext->height);

	// ͼ���С
	int frame_size = video_codeccontext->width * video_codeccontext->height;

	// ����һ�����ݰ� ��ȡһ(��Ƶ)֮֡�� ������(��Ƶ)֡������
	AVPacket* read_packct = new AVPacket;
	av_new_packet(read_packct, frame_size);

	// ��ӡ������Ƶ�ĸ�ʽ
	// av_dump_format(format_context, 0, file_name, 0);

	QPixmap pixmap(video_codeccontext->width, video_codeccontext->height);
	int get_picture = 0;
	int pic_area = video_codeccontext->width * video_codeccontext->height;
	while (true)
	{
		// ��һ֡ <0 ��˵��������
		if (av_read_frame(format_context, read_packct) < 0)
		{
			break;
		}

		read_packct->pts;

		// ��ȫ���ж� ȷ������Ƶ�������Ķ���
		if (read_packct->stream_index == video_stream_index)
		{
			// ����һ֡
			int ret = avcodec_decode_video2(video_codeccontext, frame, &get_picture, read_packct);

			if (ret < 0)
				break;
		}

		if (get_picture)
		{
			// ֡ͼ���ʽת��
			sws_scale(img_convert_ctx, frame->data, frame->linesize, 0, video_codeccontext->height, frameRGB->data, frameRGB->linesize);
		}

		QImage& image = QImage(out_buffer, video_codeccontext->width, video_codeccontext->height, QImage::Format_Grayscale8);
		QPixmap& pixmap = QPixmap::fromImage(image);
		pixmap = pixmap.scaled(m_showsize);
		emit collect_one_frame(pixmap, 0);
	}

	av_free(out_buffer);						// ��ջ�����
	av_free(frameRGB);
	avcodec_close(video_codeccontext);
	avformat_close_input(&format_context);
	emit finish_collect_frame();
}





VideoPlayer_ffmpeg_kit::VideoPlayer_ffmpeg_kit(VideoPlayer_ffmpeg* p) 
{
	frame_displayer = new QLabel(p);
	frame_displayer->setGeometry(0, 0, 900, 900);
	frame_displayer->show();

	controler = new VideoPlayer_ffmpeg_ControlPanel(p);
	controler->setGeometry(0, 750, 900, 150);
	controler->hide();
}






VideoPlayer_ffmpeg::VideoPlayer_ffmpeg(QWidget* p):
QWidget(p)
{
	m_collector = new VideoPlayer_ffmpeg_FrameCollector(this);
	connect(m_collector, &VideoPlayer_ffmpeg_FrameCollector::collect_one_frame, this, &VideoPlayer_ffmpeg::slot_show_one_frame);
	connect(m_collector, &VideoPlayer_ffmpeg_FrameCollector::finish_collect_frame, this, &VideoPlayer_ffmpeg::slot_finish_collect_frame);

	m_VideoPlayer_ffmpeg_kit = new VideoPlayer_ffmpeg_kit(this);
}

VideoPlayer_ffmpeg::~VideoPlayer_ffmpeg()
{

}

void VideoPlayer_ffmpeg::play(const QString & video_name)
{
	m_VideoPlayer_ffmpeg_kit->frame_displayer->show();
	m_VideoPlayer_ffmpeg_kit->controler->show();
	this->show();

	m_collector->set_videoname(video_name);
	m_collector->set_playspeed(1);
	m_collector->set_show_size(m_VideoPlayer_ffmpeg_kit->frame_displayer->size());
	m_collector->start();
}

void VideoPlayer_ffmpeg::enterEvent(QEvent* event)
{
	m_VideoPlayer_ffmpeg_kit->controler->show();
}
void VideoPlayer_ffmpeg::leaveEvent(QEvent* event)
{
	m_VideoPlayer_ffmpeg_kit->controler->hide();
}

void VideoPlayer_ffmpeg::slot_show_one_frame(const QPixmap& pixmap, int prog)
{
	m_VideoPlayer_ffmpeg_kit->frame_displayer->setPixmap(pixmap);
}

void VideoPlayer_ffmpeg::slot_play_or_pause()
{

}

void VideoPlayer_ffmpeg::slot_video_leap()
{

}

void VideoPlayer_ffmpeg::slot_change_playspeed()
{

}

void VideoPlayer_ffmpeg::slot_delete_video()
{

}

void VideoPlayer_ffmpeg::slot_shear_video()
{

}

void VideoPlayer_ffmpeg::slot_finish_collect_frame()
{
	m_collector->quit();
	m_collector->terminate();
	m_VideoPlayer_ffmpeg_kit->frame_displayer->hide();
	this->hide();

	emit play_finish();
}
