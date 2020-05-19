#include "VideoPlayer_ffmpeg.h"


VideoPlayer_ffmpeg_FrameCollector::VideoPlayer_ffmpeg_FrameCollector(VideoPlayer_ffmpeg* p) : 
QThread(p)
{
	m_format_context = nullptr;
	m_format_context = nullptr;
	m_video_codeccontext = nullptr;
	m_video_codec = nullptr;
	m_oriframe = nullptr;
	m_swsframe = nullptr;
	m_img_convert_ctx = nullptr;
	m_frame_buffer = nullptr;
	m_read_packct = nullptr;

	m_play_speed = 1;
	m_videostream_idx = -1;

	m_is_thred_run = false;
	m_play_status = play_status::PAUSING;
}
int VideoPlayer_ffmpeg_FrameCollector::init(const QString& video_path, QSize show_size, int playspeed)
{
	av_register_all();	// ��ʼ��ffmpeg����

	// ��һ����ʽ���� ���Ҽ��鿴"��Ƶ����"
	m_format_context = avformat_alloc_context();
	int open_format = avformat_open_input(&m_format_context, video_path.toStdString().c_str(), nullptr, nullptr);
	if (open_format < 0)
	{
		return -1;
	}

	// ���� ��ʽ���� Ѱ����Ƶ������Ƶ��֮�����Ϣ(TopV��Ŀ����Ƶһ�㶼ֻ����Ƶ��)
	if (avformat_find_stream_info(m_format_context, nullptr) < 0)
	{
		return -1;
	}

	// Ѱ����Ƶ����λ��
	m_videostream_idx = -1;
	for (int i = 0; i < m_format_context->nb_streams; ++i)
	{
		if (m_format_context->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			m_videostream_idx = i;
			break;
		}
	}
	if (-1 == m_videostream_idx)
	{
		return -1;
	}

	// �ҵ���Ƶ��
	m_videostream = m_format_context->streams[m_videostream_idx];
	if (nullptr == m_videostream)
	{
		return -1;
	}

	// �ҵ���Ƶ���Ľ���������
	m_video_codeccontext = m_videostream->codec;
	if (nullptr == m_video_codeccontext)
	{
		return -1;
	}

	// ffmpeg�ṩ��һϵ�еĽ����� ������Ǹ���id��Ҫ�õ��Ǹ�������
	m_video_codec = avcodec_find_decoder(m_video_codeccontext->codec_id);
	if (nullptr == m_video_codec)
	{
		return -1;
	}

	// �򿪽�����
	if (avcodec_open2(m_video_codeccontext, m_video_codec, nullptr) < 0)
	{
		return -1;
	}

	// ����֡�ṹ
	m_oriframe = av_frame_alloc();
	if (nullptr == m_oriframe)
	{
		return -1;
	}
	m_swsframe = av_frame_alloc();
	if (nullptr == m_swsframe)
	{
		return -1;
	}

	// ͼ���ʽת���� YUV420 -> RGB24
	m_img_convert_ctx = sws_getContext(
		m_video_codeccontext->width,
		m_video_codeccontext->height,
		m_video_codeccontext->pix_fmt,
		m_video_codeccontext->width,
		m_video_codeccontext->height,
		AV_PIX_FMT_RGB24,
		SWS_BICUBIC,
		NULL,
		NULL,
		NULL
		);
	if (nullptr == m_img_convert_ctx)
	{
		return -1;
	}
	m_image_fmt = QImage::Format_RGB888;

	// ����ת���ʽ���� һ֡��Ҫ���ֽ��� ȡ���� ��ʽ��ͼ���С
	int num_bytes = avpicture_get_size(
		AV_PIX_FMT_RGB24,
		m_video_codeccontext->width,
		m_video_codeccontext->height);
	if (num_bytes <= 0)
	{
		return -1;
	}

	// ����һ֡��Ҫ���ֽ��� ��֡����һ�������� ��װ��һ֡������
	m_frame_buffer = (uint8_t*)av_malloc(num_bytes * sizeof(uint8_t));
	if (nullptr == m_frame_buffer)
	{
		return -1;
	}

	int fill_succ = avpicture_fill((AVPicture*)m_swsframe, m_frame_buffer, AV_PIX_FMT_RGB24,
		m_video_codeccontext->width, m_video_codeccontext->height);
	if (fill_succ < 0)
	{
		return -1;
	}

	// ͼ�����
	int frame_area = m_video_codeccontext->width * m_video_codeccontext->height;

	// ����һ�����ݰ� ��ȡһ(��Ƶ)֮֡�� ������(��Ƶ)֡������
	m_read_packct = new AVPacket;
	if (av_new_packet(m_read_packct, frame_area) < 0)
	{
		return -1;
	}

	m_play_speed = playspeed;
	m_showsize = show_size;
	m_play_status = play_status::PAUSING;

	// ������Ƶ����
	double video_sumtime = m_format_context->duration / AV_TIME_BASE;
	m_video_second = (int)video_sumtime;

	return m_video_second;
}
void VideoPlayer_ffmpeg_FrameCollector::play()
{
	m_play_status = play_status::PLAYING;
	m_is_thred_run = true;
	if (!this->isRunning())
	{
		this->start();
	}
}
void VideoPlayer_ffmpeg_FrameCollector::pause()
{
	m_play_status = play_status::PAUSING;
}
void VideoPlayer_ffmpeg_FrameCollector::stop()
{
	m_is_thred_run = false;
	this->wait();
	while (this->isRunning());

	free_src();

	m_play_speed = 1;
	m_videostream_idx = -1;
	m_play_status = play_status::PAUSING;
}
void VideoPlayer_ffmpeg_FrameCollector::leap(int pos)
{

}
bool VideoPlayer_ffmpeg_FrameCollector::playing()
{
	return (m_play_status == play_status::PLAYING);
}
bool VideoPlayer_ffmpeg_FrameCollector::pausing()
{
	return (m_play_status == play_status::PAUSING);
}
void VideoPlayer_ffmpeg_FrameCollector::free_src()
{
	av_free(m_frame_buffer);						// ��ջ�����
	av_free(m_oriframe);							// ���ԭʼ֡��ʽ
	av_free(m_swsframe);							// ���ת��֡��ʽ
	av_free_packet(m_read_packct);
	avcodec_close(m_video_codeccontext);			// �رս�����
	avformat_close_input(&m_format_context);		// �ر���Ƶ����

	if (nullptr != m_read_packct)
		delete m_read_packct;

	m_format_context = nullptr;
	m_format_context = nullptr;
	m_video_codeccontext = nullptr;
	m_video_codec = nullptr;
	m_oriframe = nullptr;
	m_swsframe = nullptr;
	m_img_convert_ctx = nullptr;
	m_frame_buffer = nullptr;
	m_read_packct = nullptr;
}
void VideoPlayer_ffmpeg_FrameCollector::run()
{
	int now_second = 0;
	double time_base = av_q2d(m_videostream->time_base);
	while (true)
	{
		clock_t begin = clock();
		if (!m_is_thred_run)
		{
			break;
		}

		if (m_play_status == play_status::PAUSING)
		{
			continue;
		}

		int get_picture = 0;

		// ��һ֡ <0 ��˵��������
		if (av_read_frame(m_format_context, m_read_packct) < 0)
		{
			break;
		}

		int pts = m_read_packct->pts;

		// ��ȫ�ж� ȷ�����������Ƶ��
		if (m_read_packct->stream_index != m_videostream_idx)
			continue;

		// ����һ֡
		int ret = avcodec_decode_video2(m_video_codeccontext, m_oriframe, &get_picture, m_read_packct);
		// ��� ����ɹ�(ret > 0) ���� ��֡Ϊ�ؼ�֡(get_picture > 0) ��ȥת��
		if (ret > 0 && get_picture > 0)
		{
			// ת��һ֡
			sws_scale(m_img_convert_ctx, m_oriframe->data, m_oriframe->linesize, 0, m_video_codeccontext->height, m_swsframe->data, m_swsframe->linesize);
		}
		
		// ������ʾ
		QImage& image = QImage(m_frame_buffer, m_video_codeccontext->width, m_video_codeccontext->height, m_image_fmt);
		//image = image.scaled(m_showsize);
		image = image.scaledToWidth(m_showsize.width());
		emit collect_one_frame(image);

		int play_second = m_read_packct->pts * time_base;
		if (play_second != now_second)
		{
			emit playtime_changed(play_second);
			now_second = play_second;
		}
		clock_t end = clock();
		qDebug() << "analy one frame time : " << end - begin;
	}
	emit finish_collect_frame();
}






VideoPlayer_ffmpeg_ControlPanel_kit::VideoPlayer_ffmpeg_ControlPanel_kit(QLabel* p) :
QWidget(p)
{
	slider = new QSlider(this);
	slider->setTickPosition(QSlider::NoTicks);
	slider->setOrientation(Qt::Horizontal);
	slider->setGeometry(0, 0, p->width(), 10);

	play_or_pause_btn = new QPushButton(this);
	play_or_pause_btn->setGeometry(60, 50, 50, 50);
	play_or_pause_btn->setText("popop");

	video_sumtime = new QLabel(this);
	video_sumtime->setGeometry(120, 65, 100, 20);
	video_sumtime->setText("00:00:00");

	video_playtime = new QLabel(this);
	video_playtime->setGeometry(250, 65, 100, 20);
	video_playtime->setText("00:00:00");

	play_speed_box = new QComboBox(this);
	play_speed_box->setGeometry(400, 60, 40, 40);

	func_button_list = new QStackedWidget(this);
	delete_btn = new QPushButton(func_button_list);
	shear_btn = new QPushButton(func_button_list);
	func_button_list->setGeometry(450, 60, 40, 40);
	func_button_list->addWidget(delete_btn);
	func_button_list->addWidget(shear_btn);
}





VideoPlayer_ffmpeg_kit::VideoPlayer_ffmpeg_kit(VideoPlayer_ffmpeg* p) 
{
	frame_displayer = new QLabel(p);
	frame_displayer->setGeometry(0, 0, 900, 900);
	frame_displayer->show();

	controler = new VideoPlayer_ffmpeg_ControlPanel_kit(frame_displayer);
	controler->setGeometry(0, 750, 900, 150);
	controler->hide();
}




VideoPlayer_ffmpeg::VideoPlayer_ffmpeg(QWidget* p) :
QWidget(p)
{
	this->setFocusPolicy(Qt::ClickFocus);
	this->hide();

	m_VideoPlayer_ffmpeg_kit = new VideoPlayer_ffmpeg_kit(this);
	connect(m_VideoPlayer_ffmpeg_kit->controler->slider, &QSlider::sliderPressed, this, &VideoPlayer_ffmpeg::slot_video_leap);
	connect(m_VideoPlayer_ffmpeg_kit->controler->play_or_pause_btn, &QPushButton::clicked, this, &VideoPlayer_ffmpeg::slot_play_or_pause);

	m_collector = new VideoPlayer_ffmpeg_FrameCollector(this);
	connect(m_collector, &VideoPlayer_ffmpeg_FrameCollector::collect_one_frame, this, &VideoPlayer_ffmpeg::slot_show_one_frame);
	connect(m_collector, &VideoPlayer_ffmpeg_FrameCollector::playtime_changed, this, &VideoPlayer_ffmpeg::slot_playtime_changed);
	connect(m_collector, &VideoPlayer_ffmpeg_FrameCollector::finish_collect_frame, this, &VideoPlayer_ffmpeg::slot_finish_collect_frame);

	m_controller_always_hide = false;
}

VideoPlayer_ffmpeg::~VideoPlayer_ffmpeg()
{

}

void VideoPlayer_ffmpeg::set_media(const QString & video_path, video_or_gif sta, int playspeed)
{
	clear_videodisplayer();

	// ȷ����ʾ��ʽ
	if (sta == video_or_gif::VideoStyle)
		m_controller_always_hide = false;
	else if (sta == video_or_gif::GifStyle)
		m_controller_always_hide = true;

	// ȷ����Ƶ·����ȷ
	QFileInfo video_info(video_path);
	if (!video_info.exists())
	{
		PromptBoxInst(this)->msgbox_go(PromptBox_msgtype::Warning,
			PromptBox_btntype::None,
			QStringLiteral("û���ҵ���Ƶ��Ϣ"),
			1000, true);
		return;
	}
	const QString& abs_video_path = video_info.absoluteFilePath();

	// ��ʼ��ffmpegʹ�û��� �� ��ȡ��Ƶʱ��
	int video_sumsecond = m_collector->init(abs_video_path, m_VideoPlayer_ffmpeg_kit->frame_displayer->size(), 1);
	if (video_sumsecond < 0)
	{
		m_controller_always_hide = false;								// ffmpeg��ʼ��ʧ�� ��������ʾ�������
		PromptBoxInst(this)->msgbox_go(PromptBox_msgtype::Warning,
			PromptBox_btntype::None,
			QStringLiteral("��Ƶ��������"),
			1000, true);
		return;
	}
	
	// ��ʾʱ�����������
	QTime video_sumtime(0, 0, 0);
	video_sumtime = video_sumtime.addSecs(video_sumsecond);
	m_VideoPlayer_ffmpeg_kit->controler->video_sumtime->setText(video_sumtime.toString("hh:mm:ss"));

	// ������Ƶʱ�� ��ʼ����������ֵ��Χ
	m_VideoPlayer_ffmpeg_kit->controler->slider->setRange(0, video_sumsecond);
}

void VideoPlayer_ffmpeg::enterEvent(QEvent* event)
{
	if (m_controller_always_hide)
		return;

	m_VideoPlayer_ffmpeg_kit->controler->show();
}
void VideoPlayer_ffmpeg::leaveEvent(QEvent* event)
{
	if (m_controller_always_hide)
		return;

	m_VideoPlayer_ffmpeg_kit->controler->hide();
}
void VideoPlayer_ffmpeg::keyPressEvent(QKeyEvent* event)
{
	if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_W)
	{
		event->accept();
		slot_exit();
	}
	else
	{
		event->ignore();
	}
}

void VideoPlayer_ffmpeg::clear_videodisplayer()
{
	QImage pure_dark_image(m_VideoPlayer_ffmpeg_kit->frame_displayer->size(), QImage::Format_RGB888);
	pure_dark_image.fill(Qt::black);
	m_VideoPlayer_ffmpeg_kit->frame_displayer->setPixmap(QPixmap::fromImage(pure_dark_image));
}

void VideoPlayer_ffmpeg::slot_show_one_frame(const QImage& image)
{
	const QPixmap& toshow_pixmap = QPixmap::fromImage(image);
	m_VideoPlayer_ffmpeg_kit->frame_displayer->setPixmap(toshow_pixmap);
}
void VideoPlayer_ffmpeg::slot_playtime_changed(int sec)
{
	QTime video_playtime(0, 0, 0);
	video_playtime = video_playtime.addSecs(sec);
	m_VideoPlayer_ffmpeg_kit->controler->video_playtime->setText(video_playtime.toString("hh:mm:ss"));

	m_VideoPlayer_ffmpeg_kit->controler->slider->setValue(sec);
}

void VideoPlayer_ffmpeg::slot_play_or_pause()
{
	if (m_collector->pausing())
	{
		// ��ʼ����Ƶ��ȡ֡
		m_collector->play();
	}
	else if (m_collector->playing())
	{
		m_collector->pause();
	}
}

void VideoPlayer_ffmpeg::slot_video_leap()
{
	int pos = m_VideoPlayer_ffmpeg_kit->controler->slider->value();
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

void VideoPlayer_ffmpeg::slot_exit()
{
	m_collector->stop();
	this->hide();
}

void VideoPlayer_ffmpeg::slot_finish_collect_frame()
{
	m_collector->stop();
	emit play_finish();
}
