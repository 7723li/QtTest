#include "VideoPlayer_ffmpeg.h"


VideoPlayer_ffmpeg_FrameCollector::VideoPlayer_ffmpeg_FrameCollector(VideoPlayer_ffmpeg* p) : 
QThread(p)
{
	m_format_context = nullptr;
	m_format_context = nullptr;
	m_video_codeccontext = nullptr;
	m_video_codec = nullptr;
	m_orifmt_frame = nullptr;
	m_swsfmt_frame = nullptr;
	m_img_convert_ctx = nullptr;
	m_frame_buffer = nullptr;
	m_read_packct = nullptr;

	m_play_speed = 1;
	m_videostream_idx = -1;

	m_framenum = 0;
	m_fps = 0;
	m_second_timebase = 0;
	m_process_perframe = 0;
	m_video_msecond = 0;

	m_is_thred_run = false;
	m_play_status = play_status::STATIC;
}
videoduration_ms VideoPlayer_ffmpeg_FrameCollector::init(const QString& video_path)
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

	// ��ȡ֡��
	m_fps = av_q2d(m_videostream->r_frame_rate);

	// ����ʱ���
	m_second_timebase = av_q2d(m_videostream->time_base);

	// ���� ÿ֡�����۵�������ʱ��(��λ:ms)
	m_process_perframe = (double)1000 / (double)m_fps;

	// �ҵ���Ƶ���Ľ���������
	m_video_codeccontext = m_videostream->codec;
	if (nullptr == m_video_codeccontext)
	{
		return -1;
	}

	// ffmpeg�ṩ��һϵ�еĽ����� ������Ǹ���id��Ҫ�õ��Ǹ������� ����H264
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
	m_orifmt_frame = av_frame_alloc();
	if (nullptr == m_orifmt_frame)
	{
		return -1;
	}
	m_swsfmt_frame = av_frame_alloc();
	if (nullptr == m_swsfmt_frame)
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

	int fill_succ = avpicture_fill((AVPicture*)m_swsfmt_frame, m_frame_buffer, AV_PIX_FMT_RGB24,
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

	// ������Ƶ���볤��
	m_video_msecond = m_format_context->duration / AV_TIME_BASE * 1000;

	return m_video_msecond;
}
void VideoPlayer_ffmpeg_FrameCollector::set_showsize(QSize show_size)
{
	m_showsize = show_size;
}
void VideoPlayer_ffmpeg_FrameCollector::set_playspeed(int playspeed)
{
	m_play_speed = playspeed;
}
void VideoPlayer_ffmpeg_FrameCollector::play(int begin, int finish)
{
	m_play_status = play_status::DYNAMIC;
}
void VideoPlayer_ffmpeg_FrameCollector::pause()
{
	m_play_status = play_status::STATIC;
}
void VideoPlayer_ffmpeg_FrameCollector::stop()
{
	m_is_thred_run = false;
	this->wait();
	while (this->isRunning());
}
void VideoPlayer_ffmpeg_FrameCollector::leap(int msec)
{
	double leap_to_second = (double)msec / (double)1000;
	double leap_to_timestamp = leap_to_second / m_second_timebase;
	static int leap_flag = AVSEEK_FLAG_BACKWARD;

	int seek_res = av_seek_frame(
		m_format_context,
		m_videostream_idx,
		leap_to_timestamp,
		leap_flag);

	if (0 == read_and_show())
	{
		emit playtime_changed(m_read_packct->pts * m_second_timebase * 1000);
	}
}
bool VideoPlayer_ffmpeg_FrameCollector::playing()
{
	return (m_play_status == play_status::DYNAMIC);
}
bool VideoPlayer_ffmpeg_FrameCollector::pausing()
{
	return (m_play_status == play_status::STATIC);
}
int VideoPlayer_ffmpeg_FrameCollector::read_and_show()
{
	// ��һ֡ < 0 ��˵��������
	if (av_read_frame(m_format_context, m_read_packct) < 0)
	{
		return -1;
	}

	// ��ȫ�ж� ȷ�����������Ƶ��
	if (m_read_packct->stream_index != m_videostream_idx)
		return -2;

	// ����һ֡
	int get_picture = 0;
	int ret = avcodec_decode_video2(m_video_codeccontext, m_orifmt_frame, &get_picture, m_read_packct);
	// ��� ����ɹ�(ret > 0) ���� ��֡Ϊ�ؼ�֡(get_picture > 0) ��ȥת��
	if (ret > 0 && get_picture > 0)
	{
		// ת��һ֡��ʽ
		sws_scale(m_img_convert_ctx, m_orifmt_frame->data, m_orifmt_frame->linesize, 0, m_video_codeccontext->height, m_swsfmt_frame->data, m_swsfmt_frame->linesize);
	}

	// ����һ֡��С
	cv::Mat orisize_frame(cv::Size(m_video_codeccontext->width, m_video_codeccontext->height), CV_8UC3, m_frame_buffer);
	cv::Mat swssize_frame(cv::Size(m_showsize.width(), m_showsize.height()), CV_8UC3);
	cv::resize(orisize_frame, swssize_frame, cv::Size(swssize_frame.cols, swssize_frame.rows), 0, 0);

	// ��ʾ
	emit collect_one_frame(swssize_frame);

	return 0;
}
void VideoPlayer_ffmpeg_FrameCollector::relaese()
{
	av_free(m_frame_buffer);						// ��ջ�����
	av_free(m_orifmt_frame);							// ���ԭʼ֡��ʽ
	av_free(m_swsfmt_frame);							// ���ת��֡��ʽ
	av_free_packet(m_read_packct);
	avcodec_close(m_video_codeccontext);			// �رս�����
	avformat_close_input(&m_format_context);		// �ر���Ƶ����

	if (nullptr != m_read_packct)
		delete m_read_packct;

	m_format_context = nullptr;
	m_format_context = nullptr;
	m_video_codeccontext = nullptr;
	m_video_codec = nullptr;
	m_orifmt_frame = nullptr;
	m_swsfmt_frame = nullptr;
	m_img_convert_ctx = nullptr;
	m_frame_buffer = nullptr;
	m_read_packct = nullptr;

	m_play_speed = 1;
	m_videostream_idx = -1;
	m_play_status = play_status::STATIC;
}
void VideoPlayer_ffmpeg_FrameCollector::run()
{
	int now_second = 0, play_second = 0;
	m_is_thred_run = true;
	while (true)
	{
		clock_t begin = clock();

		// �˳��߳�
		if (!m_is_thred_run)
		{
			break;
		}

		// ��ͣ
		if (m_play_status == play_status::STATIC)
		{
			continue;
		}

		if (-1 == read_and_show())
		{
			break;
		}

		// ������һ��һ�� ����Ƶ���л�
		play_second = m_read_packct->pts * m_second_timebase;
		if (play_second != now_second)
		{
			emit playtime_changed(play_second * 1000);
			now_second = play_second;
		}

		// ������һ֡ʵ�ʴ���ʱ��(��λ:ms)
		clock_t end = clock();
		clock_t this_frame_proc_time = end - begin;

		// ʵ�ʵ�֡����ʱ�� �� ���۵�֡����ʱ�仹Ҫ�� ˵��״̬��̫�� ���ڹ�!!!
		if (this_frame_proc_time < m_process_perframe)
		{
			::Sleep(m_process_perframe - this_frame_proc_time);
		}
	}
	m_play_status = play_status::STATIC;
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
	frame_displayer->setGeometry(0, 0, 1032, 771);
	frame_displayer->show();

	controler = new VideoPlayer_ffmpeg_ControlPanel_kit(frame_displayer);
	controler->setGeometry(0, 621, 1032, 150);
	controler->hide();
}




VideoPlayer_ffmpeg::VideoPlayer_ffmpeg(QWidget* p) :
QWidget(p)
{
	qRegisterMetaType<cv::Mat>("cv::Mat");

	this->setFocusPolicy(Qt::ClickFocus);
	this->hide();

	m_VideoPlayer_ffmpeg_kit = new VideoPlayer_ffmpeg_kit(this);
	connect(m_VideoPlayer_ffmpeg_kit->controler->slider, &QSlider::sliderPressed, this, &VideoPlayer_ffmpeg::slot_slider_pressed);
	connect(m_VideoPlayer_ffmpeg_kit->controler->slider, &QSlider::sliderReleased, this, &VideoPlayer_ffmpeg::slot_slider_released);
	connect(m_VideoPlayer_ffmpeg_kit->controler->slider, &QSlider::actionTriggered, this, &VideoPlayer_ffmpeg::slot_slider_triggered);
	connect(m_VideoPlayer_ffmpeg_kit->controler->play_or_pause_btn, &QPushButton::clicked, this, &VideoPlayer_ffmpeg::slot_play_or_pause);

	// lzx 20200520 ��Ƶ������ע���¼������� ʵ�������� ���eventFilterע��
	m_VideoPlayer_ffmpeg_kit->controler->slider->installEventFilter(this);

	m_collector = new VideoPlayer_ffmpeg_FrameCollector(this);
	connect(m_collector, &VideoPlayer_ffmpeg_FrameCollector::collect_one_frame, this, &VideoPlayer_ffmpeg::slot_show_one_frame);
	connect(m_collector, &VideoPlayer_ffmpeg_FrameCollector::playtime_changed, this, &VideoPlayer_ffmpeg::slot_playtime_changed);
	connect(m_collector, &VideoPlayer_ffmpeg_FrameCollector::finish_collect_frame, this, &VideoPlayer_ffmpeg::slot_finish_collect_frame);

	m_controller_always_hide = false;
}

VideoPlayer_ffmpeg::~VideoPlayer_ffmpeg()
{

}

void VideoPlayer_ffmpeg::set_media(const QString & video_path,
	video_or_gif sta,
	int begin,
	int finish)
{
	// �����ʾ����
	clear_videodisplayer();

	set_style(sta);
	set_range(begin, finish);

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

	// ��ʼ��ffmpegʹ�û��� �� ��ȡ��Ƶʱ��(����)
	videoduration_ms video_msecond = m_collector->init(abs_video_path);
	if (video_msecond < 0)
	{
		m_controller_always_hide = false;								// ffmpeg��ʼ��ʧ�� ��������ʾ�������
		PromptBoxInst(this)->msgbox_go(PromptBox_msgtype::Warning,
			PromptBox_btntype::None,
			QStringLiteral("��Ƶ��������"),
			1000, true);
		return;
	}

	// ��ʾʱ�����������
	QTime video_sumtime(0, 0, 0, 0);
	video_sumtime = video_sumtime.addMSecs(video_msecond);
	m_VideoPlayer_ffmpeg_kit->controler->video_sumtime->setText(video_sumtime.toString("hh:mm:ss"));

	// ������Ƶʱ�� ��ʼ����������ֵ��Χ
	m_VideoPlayer_ffmpeg_kit->controler->slider->setRange(0, video_msecond);

	// ������ʾ��С����ʾ�ٶ�
	m_collector->set_showsize(m_VideoPlayer_ffmpeg_kit->frame_displayer->size());
	m_collector->set_playspeed(1);
}

void VideoPlayer_ffmpeg::set_style(video_or_gif sta)
{
	// ȷ����ʾ��ʽ
	if (sta == video_or_gif::VideoStyle)
		m_controller_always_hide = false;
	else if (sta == video_or_gif::GifStyle_without_slider)
		m_controller_always_hide = true;
}

void VideoPlayer_ffmpeg::set_range(int begin, int end)
{

}
void VideoPlayer_ffmpeg::set_begin(int being)
{

}
void VideoPlayer_ffmpeg::set_finish(int finish)
{

}

void VideoPlayer_ffmpeg::set_playspeed(int playspeed)
{
	m_collector->set_playspeed(playspeed);
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
bool VideoPlayer_ffmpeg::eventFilter(QObject* watched, QEvent* event)
{
	if (watched == m_VideoPlayer_ffmpeg_kit->controler->slider)
	{
		// 20200520 lzx �ڲ�����QSlider������� ʵ�ֲ��Ž������洦�ɵ�Ĺ���
		QSlider* slider = m_VideoPlayer_ffmpeg_kit->controler->slider;

		if (event->type() != QEvent::MouseButtonPress)
			return false;

		QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
		if (mouse_event->button() != Qt::LeftButton)
			return false;

		int duration = slider->maximum() - slider->minimum();
		int position = slider->minimum() + duration * ((double)mouse_event->x() / slider->width());
		int now_postion = slider->value();
		if (position != now_postion)
		{
			slider->setValue(position);
			if (position < now_postion)
			{
				slider->triggerAction(QAbstractSlider::SliderPageStepSub);
			}
			else
			{
				slider->triggerAction(QAbstractSlider::SliderPageStepAdd);
			}
		}
	}

	return QObject::eventFilter(watched, event);
}

void VideoPlayer_ffmpeg::clear_videodisplayer()
{
	QImage pure_dark_image(m_VideoPlayer_ffmpeg_kit->frame_displayer->size(), QImage::Format_RGB888);
	pure_dark_image.fill(Qt::black);
	m_VideoPlayer_ffmpeg_kit->frame_displayer->setPixmap(QPixmap::fromImage(pure_dark_image));
}

void VideoPlayer_ffmpeg::slot_show_one_frame(const cv::Mat image)
{
	QImage _image(image.data, image.cols, image.rows, QImage::Format_RGB888);
	const QPixmap& toshow_pixmap = QPixmap::fromImage(_image);
	m_VideoPlayer_ffmpeg_kit->frame_displayer->setPixmap(toshow_pixmap);
}
void VideoPlayer_ffmpeg::slot_playtime_changed(int msec)
{
	QTime video_playtime(0, 0, 0);
	video_playtime = video_playtime.addMSecs(msec);
	m_VideoPlayer_ffmpeg_kit->controler->video_playtime->setText(video_playtime.toString("hh:mm:ss"));

	m_VideoPlayer_ffmpeg_kit->controler->slider->setValue(msec);
}

void VideoPlayer_ffmpeg::slot_play_or_pause()
{
	if (m_collector->pausing())
	{
		// ��ʼ����Ƶ��ȡ֡
		m_collector->play();
		if (!m_collector->isRunning())
		{
			m_collector->start();
		}
	}
	else if (m_collector->playing())
	{
		m_collector->pause();
	}
}

void VideoPlayer_ffmpeg::slot_slider_pressed()
{
	m_slider_pressed_value = m_VideoPlayer_ffmpeg_kit->controler->slider->value();
	//m_collector->pause();
}	
void VideoPlayer_ffmpeg::slot_slider_triggered(int action)
{
	//m_collector->pause();
	if (action != QAbstractSlider::SliderSingleStepAdd &&		// ��굥�������ұ�
		action != QAbstractSlider::SliderSingleStepSub &&		// ��굥���������
		action != QAbstractSlider::SliderPageStepAdd &&			// �����Ҽ�ͷ
		action != QAbstractSlider::SliderPageStepSub)			// �������ͷ
	{
		return;
	}

	int value = m_VideoPlayer_ffmpeg_kit->controler->slider->value();
	m_collector->leap(value);
	//m_collector->play();
}
void VideoPlayer_ffmpeg::slot_slider_released()
{
	int value = m_VideoPlayer_ffmpeg_kit->controler->slider->value();
	if (value != m_slider_pressed_value)
	{
		m_collector->leap(value);
	}

	/*if (m_collector->pausing())
		m_collector->play();*/
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
