#include "VideoPlayer_ffmpeg.h"

static QString g_shear_videopath = "./shear.avi";

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

	m_videostream_idx = -1;

	m_framenum = 0;
	m_fps = 0;
	m_second_timebase = 0;
	m_process_perframe_1speed = 0;
	m_process_perframe = 0;
	m_video_msecond = 0;

	m_is_init = false;
	m_is_thread_run = false;
	m_is_thread_pausing = true;
	m_play_status = play_status::STATIC;

	m_runmode = RunMode::ShowVideoMode;
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
	m_process_perframe_1speed = (double)1000 / (double)m_fps;
	m_process_perframe = m_process_perframe_1speed;

	// �ҵ���Ƶ���Ľ���������
	m_video_codeccontext = m_videostream->codec;
	if (nullptr == m_video_codeccontext)
	{
		return -1;
	}

	// ����Ĭ����ʾ��С��ͼ��һ��
	m_showsize = QSize(m_video_codeccontext->width, m_video_codeccontext->height);

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
		AV_PIX_FMT_GRAY8,
		SWS_BICUBIC,
		NULL,
		NULL,
		NULL
		);
	if (nullptr == m_img_convert_ctx)
	{
		return -1;
	}
	m_image_fmt = QImage::Format_Grayscale8;

	// ����ת���ʽ���� һ֡��Ҫ���ֽ��� ȡ���� ��ʽ��ͼ���С
	int num_bytes = avpicture_get_size(
		AV_PIX_FMT_GRAY8,
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

	int fill_succ = avpicture_fill((AVPicture*)m_swsfmt_frame, m_frame_buffer, AV_PIX_FMT_GRAY8,
		m_video_codeccontext->width, m_video_codeccontext->height);
	if (fill_succ < 0)
	{
		return -1;
	}

	// ͼ�����
	int frame_area = m_video_codeccontext->width * m_video_codeccontext->height;

	// ����һ�����ݰ� ��ȡһ(��Ƶ)֮֡�� ������(��Ƶ)֡������
	m_read_packct = av_packet_alloc();	
	if (av_new_packet(m_read_packct, frame_area) < 0)
	{
		return -1;
	}

	m_is_init = true;

	// ������Ƶ���볤��
	m_video_msecond = m_format_context->duration / AV_TIME_BASE * 1000;
	return m_video_msecond;
}
void VideoPlayer_ffmpeg_FrameCollector::play(int begin, int finish)
{
	m_play_status = play_status::DYNAMIC; 
}
void VideoPlayer_ffmpeg_FrameCollector::leap(int msec)
{
	if (!m_is_init)
		return;

	play_status save_status = m_play_status;
	m_play_status = play_status::STATIC;			// ����ͣ�߳� ���������Դ��ͻ
	if (m_is_thread_run)
		while (!m_is_thread_pausing);

	double leap_to_second = (double)msec / (double)1000;
	double leap_to_timestamp = leap_to_second / m_second_timebase;
	static int leap_flag = AVSEEK_FLAG_BACKWARD;

	int seek_res = av_seek_frame(
		m_format_context,
		m_videostream_idx,
		leap_to_timestamp,
		leap_flag);

	if (seek_res < 0)
		return;

	if (av_read_frame(m_format_context, m_read_packct) < 0)
	{
		return;
	}
	while (m_read_packct->stream_index != m_videostream_idx)
	{
		if (av_read_frame(m_format_context, m_read_packct) < 0)
		{
			return;
		}
	}
	int get_picture = 0;
	int ret = avcodec_decode_video2(m_video_codeccontext, m_orifmt_frame, &get_picture, m_read_packct);
	if (ret > 0 && get_picture > 0)
	{
		sws_scale(m_img_convert_ctx, m_orifmt_frame->data, m_orifmt_frame->linesize, 0, m_video_codeccontext->height, m_swsfmt_frame->data, m_swsfmt_frame->linesize);
	}

	cv::Mat orisize_frame(cv::Size(m_video_codeccontext->width, m_video_codeccontext->height), CV_8UC1, m_frame_buffer);
	cv::Mat swssize_frame(cv::Size(m_showsize.width(), m_showsize.height()), CV_8UC1);
	cv::resize(orisize_frame, swssize_frame, cv::Size(swssize_frame.cols, swssize_frame.rows), 0, 0);

	emit show_one_frame(swssize_frame);
	emit playtime_changed(m_read_packct->pts * m_second_timebase * 1000);

	m_play_status = save_status;
}
void VideoPlayer_ffmpeg_FrameCollector::relaese()
{
	if (nullptr != m_frame_buffer)
		av_free(m_frame_buffer);					// ��ջ�����

	if (nullptr != m_orifmt_frame)
		av_free(m_orifmt_frame);					// ���ԭʼ֡��ʽ

	if (nullptr != m_swsfmt_frame)
		av_free(m_swsfmt_frame);					// ���ת��֡��ʽ

	if (nullptr != m_read_packct)
		av_free_packet(m_read_packct);				// �ͷŶ�ȡ��

	if (nullptr != m_video_codeccontext)
		avcodec_close(m_video_codeccontext);		// �رս�����

	if (nullptr != m_format_context)
		avformat_close_input(&m_format_context);	// �ر���Ƶ����

	m_format_context = nullptr;
	m_format_context = nullptr;
	m_video_codeccontext = nullptr;
	m_video_codec = nullptr;
	m_orifmt_frame = nullptr;
	m_swsfmt_frame = nullptr;
	m_img_convert_ctx = nullptr;
	m_frame_buffer = nullptr;
	m_read_packct = nullptr;

	m_is_init = false;
	m_is_thread_run = false;
	m_is_thread_pausing = true;
	m_videostream_idx = -1;
	m_play_status = play_status::STATIC;
}
void VideoPlayer_ffmpeg_FrameCollector::run()
{
	int now_second = 0, play_second = 0;
	videoduration_ms play_msecond = 0;
	m_is_thread_run = true;
	while (true)
	{
		clock_t begin = clock();
		m_is_thread_pausing = true;

		// �˳��߳�
		if (!m_is_thread_run)
		{
			break;
		}

		// ��ͣ
		if (m_play_status == play_status::STATIC)
		{
			continue;
		}

		m_is_thread_pausing = false;

		// ��һ֡ < 0 ��˵��������
		if (av_read_frame(m_format_context, m_read_packct) < 0)
		{
			break;
		}

		// ��ȫ�ж� ȷ�����������Ƶ��
		if (m_read_packct->stream_index != m_videostream_idx)
			continue;

		// ����һ֡
		int get_picture = 0;
		int ret = avcodec_decode_video2(m_video_codeccontext, m_orifmt_frame, &get_picture, m_read_packct);
		// ��� ����ɹ�(ret > 0) ���� ��֡Ϊ�ؼ�֡(get_picture > 0) ��ȥת��
		if (ret > 0 && get_picture > 0)
		{
			// ת��һ֡��ʽ
			sws_scale(m_img_convert_ctx, m_orifmt_frame->data, m_orifmt_frame->linesize, 0, m_video_codeccontext->height, m_swsfmt_frame->data, m_swsfmt_frame->linesize);
		}

		// ��AVFrame ת���� Mat
		cv::Mat orisize_frame(cv::Size(m_video_codeccontext->width, m_video_codeccontext->height), CV_8UC1, m_frame_buffer);

		// ���㵱ǰ֡�Ķ�Ӧ����ʱ��
		play_second = m_read_packct->pts * m_second_timebase;
		play_msecond = play_second * 1000;
		if (m_runmode == RunMode::ShowVideoMode)
		{
			// ����һ֡��С
			cv::Mat swssize_frame(cv::Size(m_showsize.width(), m_showsize.height()), CV_8UC1);
			cv::resize(orisize_frame, swssize_frame, cv::Size(swssize_frame.cols, swssize_frame.rows), 0, 0);

			// ��ʾ
			emit show_one_frame(swssize_frame);
			
			// ������һ��һ�� ����Ƶ���л�
			if (play_second != now_second)
			{
				emit playtime_changed(play_msecond);
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
		else if (m_runmode == RunMode::ShearVideoMode)
		{
			emit shear_one_frame(orisize_frame);
		}

		// ���Ž�β��ֵ���� �� �Ѿ������β
		if (-1 != m_video_playend && play_msecond > m_video_playend)
		{
			break;
		}
	}
	m_is_thread_run = false;
	m_is_thread_pausing = true;
	m_play_status = play_status::STATIC;

	if (m_runmode == RunMode::ShowVideoMode)
	{
		emit finish_collect_frame();
	}
	else if (m_runmode == RunMode::ShearVideoMode)
	{
		emit finish_shear();
	}
}





VideoPlayer_ffmpeg_ControlPanel_kit::VideoPlayer_ffmpeg_ControlPanel_kit(QLabel* p) :
QWidget(p)
{
	this->setStyleSheet("{background-color:white;}");

	play_or_pause_btn = new QPushButton(this);
	play_or_pause_btn->setGeometry(60, 50, 50, 50);
	play_or_pause_btn->setText("popop");

	video_sumtime = new QLabel(this);
	video_sumtime->setGeometry(120, 65, 100, 20);
	video_sumtime->setText("00:00:00");

	video_playtime = new QLabel(this);
	video_playtime->setGeometry(250, 65, 100, 20);
	video_playtime->setText("00:00:00");

	playspeed_box = new QComboBox(this);
	playspeed_box->setGeometry(400, 60, 40, 40);

	func_button_list = new QStackedWidget(this);

	delete_btn = new QPushButton(func_button_list);
	delete_btn->setIcon(QIcon(".\\Resources\\icon\\delete.png")); 
	delete_btn->setGeometry(450, 60, 40, 40);

	shear_btn = new QPushButton(func_button_list);
	shear_btn->setIcon(QIcon(".\\Resources\\icon\\cut.png"));
	shear_btn->setGeometry(450, 60, 40, 40);

	func_button_list->setGeometry(450, 60, 40, 40);
	func_button_list->addWidget(delete_btn);
	func_button_list->addWidget(shear_btn);
	func_button_list->setCurrentWidget(shear_btn);

	comfirm_shear_btn = new QPushButton(this);
	comfirm_shear_btn->setText(QStringLiteral("ȷ��"));
	comfirm_shear_btn->setGeometry(500, 60, 60, 27);
	comfirm_shear_btn->hide();

	cancle_shear_btn = new QPushButton(this);
	cancle_shear_btn->setText(QStringLiteral("ȡ��"));
	cancle_shear_btn->setGeometry(600, 60, 60, 27);
	cancle_shear_btn->hide();
}





VideoPlayer_ffmpeg_kit::VideoPlayer_ffmpeg_kit(VideoPlayer_ffmpeg* p) 
{
	frame_displayer = new QLabel(p);
	frame_displayer->setGeometry(0, 0, 1032, 771);
	frame_displayer->show();

	slider_background = new QWidget(p);
	slider_background->setObjectName("slider_background ");
	slider_background->setStyleSheet("QWidget#slider_background {background-color: rgb(0, 0, 0);}");
	slider_background->setGeometry(0, 611, 1032, 14);
	slider_background->show();

	slider = new VidSlider(slider_background);
	slider->setTickPosition(QSlider::NoTicks);
	slider->setOrientation(Qt::Horizontal);
	slider->setGeometry(0, 0, slider_background->width(), slider_background->height());
	slider->show();

	controler = new VideoPlayer_ffmpeg_ControlPanel_kit(frame_displayer);
	controler->setGeometry(0, 621, 1032, 150);
	controler->show();
}




VideoPlayer_ffmpeg::VideoPlayer_ffmpeg(QWidget* p) :
QWidget(p),
m_playspeed_choosen({ 0.2, 0.5, 1, 1.5, 2 }),
m_playspeed_1_index(2)
{
	qRegisterMetaType<cv::Mat>("cv::Mat");

	this->setFocusPolicy(Qt::ClickFocus);
	this->hide();

	m_VideoPlayer_ffmpeg_kit = new VideoPlayer_ffmpeg_kit(this);
	connect(m_VideoPlayer_ffmpeg_kit->controler->play_or_pause_btn, &QPushButton::clicked, this, &VideoPlayer_ffmpeg::slot_play_or_pause);

	// lzx 20200520 ��Ƶ������ע���¼������� ʵ�������� ���eventFilterע��
	connect(m_VideoPlayer_ffmpeg_kit->slider, 
		&VidSlider::slider_motivated, 
		this, 
		&VideoPlayer_ffmpeg::slot_slider_triggered);
	//m_VideoPlayer_ffmpeg_kit->slider->installEventFilter(this);
	m_VideoPlayer_ffmpeg_kit->slider->setCursor(QCursor(Qt::PointingHandCursor));

	init_playspeed_box();

	connect(m_VideoPlayer_ffmpeg_kit->controler->shear_btn, &QPushButton::clicked, this, &VideoPlayer_ffmpeg::slot_ready_shear_video);
	connect(m_VideoPlayer_ffmpeg_kit->controler->comfirm_shear_btn, &QPushButton::clicked, this, &VideoPlayer_ffmpeg::slot_confirm_shear_video);
	connect(m_VideoPlayer_ffmpeg_kit->controler->cancle_shear_btn, &QPushButton::clicked, this, &VideoPlayer_ffmpeg::slot_cancle_shear_video);

	m_collector = new VideoPlayer_ffmpeg_FrameCollector(this);
	connect(m_collector, &VideoPlayer_ffmpeg_FrameCollector::show_one_frame, this, &VideoPlayer_ffmpeg::slot_show_collect_one_frame);
	connect(m_collector, &VideoPlayer_ffmpeg_FrameCollector::finish_collect_frame, this, &VideoPlayer_ffmpeg::slot_finish_show_frame);
	connect(m_collector, &VideoPlayer_ffmpeg_FrameCollector::shear_one_frame, this, &VideoPlayer_ffmpeg::slot_shear_collect_one_frame);
	connect(m_collector, &VideoPlayer_ffmpeg_FrameCollector::finish_shear, this, &VideoPlayer_ffmpeg::slot_shear_finish);
	connect(m_collector, &VideoPlayer_ffmpeg_FrameCollector::playtime_changed, this, &VideoPlayer_ffmpeg::slot_playtime_changed);

	m_controller_always_hide = false;
	m_video_exist = false;
	m_replay_after_finish = false;

	m_playbeginms = 0;
	m_playendms = -1;
}
void VideoPlayer_ffmpeg::init_playspeed_box()
{
	if (m_VideoPlayer_ffmpeg_kit->controler->playspeed_box->count() > 0)
	{
		m_VideoPlayer_ffmpeg_kit->controler->playspeed_box->clear();
	}

	int playspeed_num = m_playspeed_choosen.size();
	for (int i = 0; i < playspeed_num; ++i)
	{
		QString choosen = QString::number(m_playspeed_choosen[i], 10, 1) + QStringLiteral("����");
		m_VideoPlayer_ffmpeg_kit->controler->playspeed_box->insertItem(i, choosen);
	}
	m_VideoPlayer_ffmpeg_kit->controler->playspeed_box->setCurrentIndex(m_playspeed_1_index);

	connect(m_VideoPlayer_ffmpeg_kit->controler->playspeed_box,
		SIGNAL(currentIndexChanged(int)),
		this,
		SLOT(slot_playspeed_changed(int)));
}

VideoPlayer_ffmpeg::set_media_status VideoPlayer_ffmpeg::set_media(
	const QString & video_path, 
	video_or_gif sta, 
	videoduration_ms beginms,
	videoduration_ms finishms)
{
	// �����ʾ����
	clear_videodisplayer();

	// ȷ����Ƶ·����ȷ
	QFileInfo video_info(video_path);
	m_abs_videopath = video_info.absoluteFilePath();
	m_video_exist = video_info.exists();
	if (!m_video_exist)
	{
		return set_media_status::NoVideoMessage;
	}

	// ��ʼ��ffmpegʹ�û��� �� ��ȡ��Ƶʱ��(����)
	videoduration_ms video_msecond = m_collector->init(m_abs_videopath);
	if (video_msecond < 0)
	{
		m_controller_always_hide = false;								// ffmpeg��ʼ��ʧ�� ��������ʾ�������
		return set_media_status::VideoAnalysisFailed;
	}

	// ������ʾ��ʽ
	set_style(sta);

	// ������ʾ��Χ
	set_playrange(beginms, finishms);

	// ����Ĭ��ʹ��1���ٲ���
	m_VideoPlayer_ffmpeg_kit->controler->playspeed_box->setCurrentIndex(m_playspeed_1_index);

	// ������ʾ��С����ʾ�ٶ�
	m_collector->set_showsize(m_VideoPlayer_ffmpeg_kit->frame_displayer->size());

	// ��ʾʱ�����������
	QTime video_sumtime(0, 0, 0, 0);
	video_sumtime = video_sumtime.addMSecs(video_msecond);
	m_VideoPlayer_ffmpeg_kit->controler->video_sumtime->setText(video_sumtime.toString("hh:mm:ss"));

	// ������Ƶʱ�� ��ʼ����������ֵ��Χ
	m_VideoPlayer_ffmpeg_kit->slider->setRange(0, video_msecond);

	return set_media_status::Succeed;
}

void VideoPlayer_ffmpeg::set_style(video_or_gif sta)
{
	// ȷ����ʾ��ʽ
	if (sta == video_or_gif::VideoStyle)
		m_controller_always_hide = false;
	else if (sta == video_or_gif::GifStyle_without_slider)
		m_controller_always_hide = true;
}

void VideoPlayer_ffmpeg::set_playrange(videoduration_ms beginms, videoduration_ms endms)
{
	m_playbeginms = beginms;
	m_playendms = endms;
	m_collector->set_playrange(m_playbeginms, m_playendms);
}
void VideoPlayer_ffmpeg::set_playbegin(videoduration_ms beginms)
{
	m_playbeginms = beginms;
	m_collector->set_playrange(m_playbeginms, m_playendms);

}
void VideoPlayer_ffmpeg::set_playend(videoduration_ms endms)
{
	m_playendms = endms;
	m_collector->set_playrange(m_playbeginms, m_playendms);
}

void VideoPlayer_ffmpeg::set_playspeed(int playspeed)
{
	m_collector->set_playspeed(playspeed);
}

void VideoPlayer_ffmpeg::set_replay(bool replay)
{
	m_replay_after_finish = replay;
	if (m_replay_after_finish)
	{
		if (m_collector->is_pause() || m_collector->is_end())
		{
			m_collector->leap(m_playbeginms);
			m_collector->play();
			m_collector->start();
		}
	}
}

void VideoPlayer_ffmpeg::enterEvent(QEvent* event)
{
	if (m_controller_always_hide)
		return;

	m_VideoPlayer_ffmpeg_kit->slider_background->show();
	m_VideoPlayer_ffmpeg_kit->slider->show();
	m_VideoPlayer_ffmpeg_kit->controler->show();
}
void VideoPlayer_ffmpeg::leaveEvent(QEvent* event)
{
	if (m_controller_always_hide)
		return;

	m_VideoPlayer_ffmpeg_kit->slider_background->hide();
	m_VideoPlayer_ffmpeg_kit->slider->hide();
	m_VideoPlayer_ffmpeg_kit->controler->hide();
}
void VideoPlayer_ffmpeg::keyPressEvent(QKeyEvent* event)
{
	if (event->modifiers() == Qt::ControlModifier && event->key() == Qt::Key_W)
	{
		event->accept();
		slot_exit();
		this->hide();
	}
	else
	{
		event->ignore();
	}
}
bool VideoPlayer_ffmpeg::eventFilter(QObject* watched, QEvent* event)
{
// 	if (watched == m_VideoPlayer_ffmpeg_kit->slider)
// 	{
// 		// 20200520 lzx �ڲ�����QSlider������� ʵ�ֲ��Ž������洦�ɵ�Ĺ���
// 		QSlider* slider = m_VideoPlayer_ffmpeg_kit->slider;
// 
// 		if (event->type() == QEvent::MouseButtonRelease)
// 		{
// 			QMouseEvent* mouse_event = static_cast<QMouseEvent*>(event);
// 			if (mouse_event->button() != Qt::LeftButton)
// 				return false;
// 
// 			int duration = slider->maximum() - slider->minimum();
// 			int position = slider->minimum() + duration * ((double)mouse_event->x() / slider->width());
// 			int now_postion = slider->value();
// 			if (position != now_postion)
// 			{
// 				slider->setValue(position);
// 				if (position < now_postion)
// 				{
// 					emit slider_motivated(QAbstractSlider::SliderPageStepSub);
// 				}
// 				else
// 				{
// 					emit slider_motivated(QAbstractSlider::SliderPageStepAdd);
// 				}
// 			}
// 		}
// 		else if (event->type() == QEvent::KeyPress)
// 		{
// 			const int step = 1;
// 			QKeyEvent* key_event = static_cast<QKeyEvent*>(event);
// 			if (key_event->key() == Qt::Key_Left)
// 			{
// 				int now_value = slider->value();
// 				if (now_value - step >= slider->minimum())
// 				{
// 					slider->setValue(now_value - step);
// 					emit slider_motivated(QAbstractSlider::SliderSingleStepSub);
// 				}
// 			}
// 			else if (key_event->key() == Qt::Key_Right)
// 			{
// 				int now_value = slider->value();
// 				if (now_value + step <= slider->maximum())
// 				{
// 					slider->setValue(now_value + step);
// 					emit slider_motivated(QAbstractSlider::SliderSingleStepAdd);
// 				}
// 			}
// 		}		
// 	}

	return QObject::eventFilter(watched, event);
}

void VideoPlayer_ffmpeg::clear_videodisplayer()
{
	QImage pure_dark_image(m_VideoPlayer_ffmpeg_kit->frame_displayer->size(), QImage::Format_Grayscale8);
	pure_dark_image.fill(Qt::black);
	m_VideoPlayer_ffmpeg_kit->frame_displayer->setPixmap(QPixmap::fromImage(pure_dark_image));
}

void VideoPlayer_ffmpeg::slot_show_collect_one_frame(const cv::Mat image)
{
	QImage _image(image.data, image.cols, image.rows, QImage::Format_Grayscale8);
	const QPixmap& toshow_pixmap = QPixmap::fromImage(_image);
	m_VideoPlayer_ffmpeg_kit->frame_displayer->setPixmap(toshow_pixmap);
}
void VideoPlayer_ffmpeg::slot_finish_show_frame()
{
	if (m_replay_after_finish)
	{
		m_collector->leap(m_playbeginms);
		m_collector->play();
		m_collector->start();
	}
	else
	{
		emit play_finish();
	}
}

void VideoPlayer_ffmpeg::slot_playtime_changed(int msec)
{
	QTime video_playtime(0, 0, 0);
	video_playtime = video_playtime.addMSecs(msec);
	m_VideoPlayer_ffmpeg_kit->controler->video_playtime->setText(video_playtime.toString("hh:mm:ss"));

	m_VideoPlayer_ffmpeg_kit->slider->setValue(msec);
}

void VideoPlayer_ffmpeg::slot_play_or_pause()
{
	if (!m_video_exist)
		return;

	if (m_collector->is_pause())
	{
		// ��ʼ����Ƶ��ȡ֡
		m_collector->play();
		if (!m_collector->isRunning())
		{
			m_collector->start();
		}
	}
	else if (m_collector->is_playing())
	{
		m_collector->pause();
	}
	else if (m_collector->is_end())
	{
		m_collector->leap(m_playbeginms);
		m_collector->play();
		m_collector->start();
	}
}

void VideoPlayer_ffmpeg::slot_slider_triggered()
{
	if (!m_video_exist)
		return;

	//if (action != QAbstractSlider::SliderSingleStepAdd &&		// ��굥�������ұ�
	//	action != QAbstractSlider::SliderSingleStepSub &&		// ��굥���������
	//	action != QAbstractSlider::SliderPageStepAdd &&			// �����Ҽ�ͷ
	//	action != QAbstractSlider::SliderPageStepSub)			// �������ͷ
	//{
	//	return;
	//}

	int value = m_VideoPlayer_ffmpeg_kit->slider->value();
	m_collector->leap(value);
}

void VideoPlayer_ffmpeg::slot_playspeed_changed(int box_idx)
{
	double change_play_speed = m_playspeed_choosen[box_idx];
	m_collector->set_playspeed(change_play_speed);
}

void VideoPlayer_ffmpeg::slot_delete_video()
{

}

void VideoPlayer_ffmpeg::slot_ready_shear_video()
{
	// lzx TODO �� qjl�� slider�л�ȡ��ȡλ�� left && right
	int left_value = 1000;
	int right_value = 4000;

	m_playbeginms = left_value;
	m_playendms = right_value;
	m_collector->set_playrange(m_playbeginms, m_playendms);

	m_VideoPlayer_ffmpeg_kit->controler->cancle_shear_btn->show();
	m_VideoPlayer_ffmpeg_kit->controler->comfirm_shear_btn->show();

	m_VideoPlayer_ffmpeg_kit->slider->Slot_ShowVesselQualitySegment();
}
void VideoPlayer_ffmpeg::slot_cancle_shear_video()
{
	m_VideoPlayer_ffmpeg_kit->controler->cancle_shear_btn->hide();
	m_VideoPlayer_ffmpeg_kit->controler->comfirm_shear_btn->hide();

	m_VideoPlayer_ffmpeg_kit->slider->Slot_HideVesselQualitySegment();
	m_VideoPlayer_ffmpeg_kit->slider->CancleCutVideo();
}
void VideoPlayer_ffmpeg::slot_confirm_shear_video()
{
	m_VideoPlayer_ffmpeg_kit->controler->cancle_shear_btn->hide();
	m_VideoPlayer_ffmpeg_kit->controler->comfirm_shear_btn->hide();

	m_VideoPlayer_ffmpeg_kit->slider->Slot_HideVesselQualitySegment();

	m_collector->stop();
	m_collector->set_playrange(m_playbeginms, m_playendms);

	int w = m_collector->get_width();
	int h = m_collector->get_height();
	m_writer.open(g_shear_videopath.toStdString().c_str(),
		CV_FOURCC('M', 'J', 'P', 'G'),
		m_collector->get_fps(),
		cv::Size(w, h), false
		);

	m_collector->set_runmode(VideoPlayer_ffmpeg_FrameCollector::RunMode::ShearVideoMode);
	m_collector->play();
	m_collector->start();
}
void VideoPlayer_ffmpeg::slot_shear_collect_one_frame(cv::Mat image)
{
	if (m_writer.isOpened())
	{
		m_writer.write(image);
	}
}
void VideoPlayer_ffmpeg::slot_shear_finish()
{
	m_writer.release();

	m_playbeginms = 0;
	m_playendms = -1;
	m_collector->set_playrange(m_playbeginms, m_playendms);
	m_collector->set_runmode(VideoPlayer_ffmpeg_FrameCollector::RunMode::ShowVideoMode);
	m_collector->play();
	m_collector->start();
}

void VideoPlayer_ffmpeg::slot_exit()
{
	m_replay_after_finish = false;
	m_collector->stop();
	m_collector->relaese();
}
