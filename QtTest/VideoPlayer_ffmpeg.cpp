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
	av_register_all();	// 初始化ffmpeg环境

	// 打开一个格式内容 并右键查看"视频属性"
	m_format_context = avformat_alloc_context();
	int open_format = avformat_open_input(&m_format_context, video_path.toStdString().c_str(), nullptr, nullptr);
	if (open_format < 0)
	{
		return -1;
	}

	// 根据 格式内容 寻找视频流、音频流之类的信息(TopV项目的视频一般都只有视频流)
	if (avformat_find_stream_info(m_format_context, nullptr) < 0)
	{
		return -1;
	}

	// 寻找视频流的位置
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

	// 找到视频流
	m_videostream = m_format_context->streams[m_videostream_idx];
	if (nullptr == m_videostream)
	{
		return -1;
	}

	// 获取帧率
	m_fps = av_q2d(m_videostream->r_frame_rate);

	// 计算时间基
	m_second_timebase = av_q2d(m_videostream->time_base);

	// 计算 每帧的理论单独处理时间(单位:ms)
	m_process_perframe_1speed = (double)1000 / (double)m_fps;
	m_process_perframe = m_process_perframe_1speed;

	// 找到视频流的解码器内容
	m_video_codeccontext = m_videostream->codec;
	if (nullptr == m_video_codeccontext)
	{
		return -1;
	}

	// 设置默认显示大小与图像一致
	m_showsize = QSize(m_video_codeccontext->width, m_video_codeccontext->height);

	// ffmpeg提供了一系列的解码器 这里就是根据id找要用的那个解码器 比如H264
	m_video_codec = avcodec_find_decoder(m_video_codeccontext->codec_id);
	if (nullptr == m_video_codec)
	{
		return -1;
	}

	// 打开解码器
	if (avcodec_open2(m_video_codeccontext, m_video_codec, nullptr) < 0)
	{
		return -1;
	}

	// 申请帧结构
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

	// 图像格式转码器 YUV420 -> RGB24
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

	// 根据转存格式计算 一帧需要的字节数 取决于 格式、图像大小
	int num_bytes = avpicture_get_size(
		AV_PIX_FMT_GRAY8,
		m_video_codeccontext->width,
		m_video_codeccontext->height);
	if (num_bytes <= 0)
	{
		return -1;
	}

	// 根据一帧需要的字节数 给帧申请一个缓冲区 来装载一帧的内容
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

	// 图像面积
	int frame_area = m_video_codeccontext->width * m_video_codeccontext->height;

	// 分配一个数据包 读取一(视频)帧之后 用来存(视频)帧的数据
	m_read_packct = av_packet_alloc();	
	if (av_new_packet(m_read_packct, frame_area) < 0)
	{
		return -1;
	}

	m_is_init = true;

	// 计算视频毫秒长度
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
	m_play_status = play_status::STATIC;			// 先暂停线程 避免造成资源冲突
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
		av_free(m_frame_buffer);					// 清空缓冲区

	if (nullptr != m_orifmt_frame)
		av_free(m_orifmt_frame);					// 清空原始帧格式

	if (nullptr != m_swsfmt_frame)
		av_free(m_swsfmt_frame);					// 清空转换帧格式

	if (nullptr != m_read_packct)
		av_free_packet(m_read_packct);				// 释放读取包

	if (nullptr != m_video_codeccontext)
		avcodec_close(m_video_codeccontext);		// 关闭解码器

	if (nullptr != m_format_context)
		avformat_close_input(&m_format_context);	// 关闭视频属性

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

		// 退出线程
		if (!m_is_thread_run)
		{
			break;
		}

		// 暂停
		if (m_play_status == play_status::STATIC)
		{
			continue;
		}

		m_is_thread_pausing = false;

		// 读一帧 < 0 就说明读完了
		if (av_read_frame(m_format_context, m_read_packct) < 0)
		{
			break;
		}

		// 安全判断 确保解码的是视频流
		if (m_read_packct->stream_index != m_videostream_idx)
			continue;

		// 解码一帧
		int get_picture = 0;
		int ret = avcodec_decode_video2(m_video_codeccontext, m_orifmt_frame, &get_picture, m_read_packct);
		// 如果 解码成功(ret > 0) 并且 该帧为关键帧(get_picture > 0) 才去转码
		if (ret > 0 && get_picture > 0)
		{
			// 转码一帧格式
			sws_scale(m_img_convert_ctx, m_orifmt_frame->data, m_orifmt_frame->linesize, 0, m_video_codeccontext->height, m_swsfmt_frame->data, m_swsfmt_frame->linesize);
		}

		// 把AVFrame 转换成 Mat
		cv::Mat orisize_frame(cv::Size(m_video_codeccontext->width, m_video_codeccontext->height), CV_8UC1, m_frame_buffer);

		// 计算当前帧的对应播放时间
		play_second = m_read_packct->pts * m_second_timebase;
		play_msecond = play_second * 1000;
		if (m_runmode == RunMode::ShowVideoMode)
		{
			// 缩放一帧大小
			cv::Mat swssize_frame(cv::Size(m_showsize.width(), m_showsize.height()), CV_8UC1);
			cv::resize(orisize_frame, swssize_frame, cv::Size(swssize_frame.cols, swssize_frame.rows), 0, 0);

			// 显示
			emit show_one_frame(swssize_frame);
			
			// 进度条一秒一换 避免频繁切换
			if (play_second != now_second)
			{
				emit playtime_changed(play_msecond);
				now_second = play_second;
			}

			// 计算这一帧实际处理时间(单位:ms)
			clock_t end = clock();
			clock_t this_frame_proc_time = end - begin;

			// 实际单帧处理时间 比 理论单帧处理时间还要长 说明状态不太对 有内鬼!!!
			if (this_frame_proc_time < m_process_perframe)
			{
				::Sleep(m_process_perframe - this_frame_proc_time);
			}
		}
		else if (m_runmode == RunMode::ShearVideoMode)
		{
			emit shear_one_frame(orisize_frame);
		}

		// 播放结尾数值合理 且 已经到达结尾
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

	playspeed_choose_btn = new QPushButton(this);
	playspeed_choose_btn->setGeometry(400, 60, 80, 40);

	playspeed_list = new QListWidget(p);
	playspeed_list->setGeometry(400, 571, 80, 120);
	playspeed_list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	playspeed_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	playspeed_list->raise();
	playspeed_list->hide();

	func_button_list = new QStackedWidget(this);
	func_button_list->setGeometry(490, 60, 40, 40);

	delete_btn = new QPushButton(func_button_list);
	delete_btn->setIcon(QIcon(".\\Resources\\icon\\delete.png")); 
	delete_btn->setGeometry(func_button_list->geometry());

	shear_btn = new QPushButton(func_button_list);
	shear_btn->setIcon(QIcon(".\\Resources\\icon\\cut.png"));
	shear_btn->setGeometry(func_button_list->geometry());

	func_button_list->addWidget(delete_btn);
	func_button_list->addWidget(shear_btn);
	func_button_list->setCurrentWidget(shear_btn);

	comfirm_shear_btn = new QPushButton(this);
	comfirm_shear_btn->setText(QStringLiteral("确认"));
	comfirm_shear_btn->setGeometry(500, 60, 60, 27);
	comfirm_shear_btn->hide();

	cancle_shear_btn = new QPushButton(this);
	cancle_shear_btn->setText(QStringLiteral("取消"));
	cancle_shear_btn->setGeometry(600, 60, 60, 27);
	cancle_shear_btn->hide();
}





VideoPlayer_ffmpeg_kit::VideoPlayer_ffmpeg_kit(VideoPlayer_ffmpeg* p) 
{
	frame_displayer = new QLabel(p);
	frame_displayer->setGeometry(0, 0, 1032, 771);
	frame_displayer->raise();
	frame_displayer->show();

	slider = new VidSlider(frame_displayer);
	slider->setTickPosition(QSlider::NoTicks);
	slider->setOrientation(Qt::Horizontal);
	slider->setGeometry(0, 611, 1032, 14);
	slider->raise();
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

	// lzx 20200520 视频进度条注册事件过滤器 实现随意点击 详见eventFilter注释
	connect(m_VideoPlayer_ffmpeg_kit->slider, 
		&VidSlider::slider_motivated, 
		this, 
		&VideoPlayer_ffmpeg::slot_slider_motivated);
	connect(m_VideoPlayer_ffmpeg_kit->slider,
		&VidSlider::blade_motivated,
		this,
		&VideoPlayer_ffmpeg::slot_blade_motivated);
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
	if (m_VideoPlayer_ffmpeg_kit->controler->playspeed_list->count() > 0)
	{
		m_VideoPlayer_ffmpeg_kit->controler->playspeed_list->clear();
	}

	int playspeed_num = m_playspeed_choosen.size();
	for (int i = 0; i < playspeed_num; ++i)
	{
		QString choosen = QString::number(m_playspeed_choosen[i], 10, 1) + QStringLiteral("倍速");
		m_VideoPlayer_ffmpeg_kit->controler->playspeed_list->insertItem(i, choosen);

		QListWidgetItem* new_item = m_VideoPlayer_ffmpeg_kit->controler->playspeed_list->item(i);
		new_item->setToolTip(choosen);
		new_item->setTextAlignment(Qt::AlignCenter);

		if (m_playspeed_choosen[i] == 1)
		{
			m_VideoPlayer_ffmpeg_kit->controler->playspeed_choose_btn->setText(choosen);
		}
	}

	connect(m_VideoPlayer_ffmpeg_kit->controler->playspeed_choose_btn,
		&QPushButton::clicked,
		this,
		&VideoPlayer_ffmpeg::slot_show_playspeed_choice);

	connect(m_VideoPlayer_ffmpeg_kit->controler->playspeed_list,
		&QListWidget::itemClicked,
		this,
		&VideoPlayer_ffmpeg::slot_playspeed_changed);
}

VideoPlayer_ffmpeg::set_media_status VideoPlayer_ffmpeg::set_media(
	const QString & video_path, 
	video_or_gif sta, 
	videoduration_ms beginms,
	videoduration_ms finishms)
{
	// 清空显示区域
	clear_videodisplayer();

	// 确保视频路径正确
	QFileInfo video_info(video_path);
	m_abs_videopath = video_info.absoluteFilePath();
	m_video_exist = video_info.exists();
	if (!m_video_exist)
	{
		return set_media_status::NoVideoMessage;
	}

	// 初始化ffmpeg使用环境 并 获取视频时长(毫秒)
	videoduration_ms video_msecond = m_collector->init(m_abs_videopath);
	if (video_msecond < 0)
	{
		m_controller_always_hide = false;								// ffmpeg初始化失败 不允许显示控制面板
		return set_media_status::VideoAnalysisFailed;
	}

	// 设置显示样式
	set_style(sta);

	// 设置显示范围
	set_playrange(beginms, finishms);

	//// 设置默认使用1倍速播放
	//m_VideoPlayer_ffmpeg_kit->controler->playspeed_list->setCurrentIndex(m_playspeed_1_index);

	// 设置显示大小和显示速度
	m_collector->set_showsize(m_VideoPlayer_ffmpeg_kit->frame_displayer->size());

	// 显示时长到控制面板
	QTime video_sumtime(0, 0, 0, 0);
	video_sumtime = video_sumtime.addMSecs(video_msecond);
	m_VideoPlayer_ffmpeg_kit->controler->video_sumtime->setText(video_sumtime.toString("hh:mm:ss"));

	// 根据视频时长 初始化进度条数值范围
	m_VideoPlayer_ffmpeg_kit->slider->setRange(0, video_msecond);
	m_VideoPlayer_ffmpeg_kit->slider->SetVideoTime(video_msecond);

	return set_media_status::Succeed;
}

void VideoPlayer_ffmpeg::set_style(video_or_gif sta)
{
	// 确认显示样式
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

	m_VideoPlayer_ffmpeg_kit->slider->show();
	m_VideoPlayer_ffmpeg_kit->controler->show();
}
void VideoPlayer_ffmpeg::leaveEvent(QEvent* event)
{
	if (m_controller_always_hide)
		return;

	if (m_VideoPlayer_ffmpeg_kit->controler->playspeed_list->isVisible())
		return;

	m_VideoPlayer_ffmpeg_kit->slider->hide();
	m_VideoPlayer_ffmpeg_kit->controler->hide();
}

/* lzx 20200525 播放器的键盘(关闭播放器)事件仅限调试时使用 */
#ifdef _DEBUG
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

void VideoPlayer_ffmpeg::mousePressEvent(QMouseEvent* event)
{
	QObject* clicked_widget = this->childAt(event->pos());
	if (nullptr == clicked_widget ||
		m_VideoPlayer_ffmpeg_kit->controler->playspeed_list == clicked_widget)
	{
		return;
	}

	if (m_VideoPlayer_ffmpeg_kit->controler->playspeed_list->isVisible())
		m_VideoPlayer_ffmpeg_kit->controler->playspeed_list->hide();
}

#endif

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

void VideoPlayer_ffmpeg::slot_show_playspeed_choice()
{
	bool choice_visible = m_VideoPlayer_ffmpeg_kit->controler->playspeed_list->isVisible();
	m_VideoPlayer_ffmpeg_kit->controler->playspeed_list->setVisible(!choice_visible);
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
		// 开始从视频中取帧
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

void VideoPlayer_ffmpeg::slot_slider_motivated()
{
	if (!m_video_exist)
		return;

	int value = m_VideoPlayer_ffmpeg_kit->slider->value();
	m_collector->leap(value);
}
void VideoPlayer_ffmpeg::slot_blade_motivated()
{
	// 从slider截取刀片位置发送变化 重新获取起始与结束
	int left_value = 0;
	int right_value = -1;
	m_VideoPlayer_ffmpeg_kit->slider->GetCutTimeRange(left_value, right_value);

	set_replay(false);

	m_playbeginms = left_value;
	m_playendms = right_value;
	m_collector->set_playrange(m_playbeginms, m_playendms);

	set_replay(true);
}

void VideoPlayer_ffmpeg::slot_playspeed_changed(QListWidgetItem* box_idx)
{
	if (nullptr == box_idx)
		return;

	QListWidget* list = m_VideoPlayer_ffmpeg_kit->controler->playspeed_list;
	if (nullptr == list)
		return;

	list->hide();
	
	int i = 0;
	for (i = 0; i < list->count(); ++i)
	{
		if (box_idx == list->item(i))
			break;
	}

	int idx = (i >= list->count() ?  0 : i);
	
	double change_play_speed = m_playspeed_choosen[idx];
	m_collector->set_playspeed(change_play_speed);
	m_VideoPlayer_ffmpeg_kit->controler->playspeed_choose_btn->setText(box_idx->text());
}

void VideoPlayer_ffmpeg::slot_delete_video()
{

}

void VideoPlayer_ffmpeg::slot_ready_shear_video()
{
	// 从slider获取截取位置 left && right
	int left_value = 0;
	int right_value = -1;
	m_VideoPlayer_ffmpeg_kit->slider->GetCutTimeRange(left_value, right_value);

	m_playbeginms = left_value;
	m_playendms = right_value;
	m_collector->set_playrange(m_playbeginms, m_playendms);
	set_replay(true);

	m_VideoPlayer_ffmpeg_kit->controler->cancle_shear_btn->show();
	m_VideoPlayer_ffmpeg_kit->controler->comfirm_shear_btn->show();

	m_VideoPlayer_ffmpeg_kit->slider->Begin_cutout_video();
}
void VideoPlayer_ffmpeg::slot_cancle_shear_video()
{
	m_VideoPlayer_ffmpeg_kit->controler->cancle_shear_btn->hide();
	m_VideoPlayer_ffmpeg_kit->controler->comfirm_shear_btn->hide();

	m_playbeginms = 0;
	m_playendms = -1;
	m_collector->set_playrange(m_playbeginms, m_playendms);
	set_replay(false);

	m_VideoPlayer_ffmpeg_kit->slider->Finish_cutout_video();
	m_VideoPlayer_ffmpeg_kit->slider->CancleCutVideo();
}
void VideoPlayer_ffmpeg::slot_confirm_shear_video()
{
	m_VideoPlayer_ffmpeg_kit->controler->cancle_shear_btn->hide();
	m_VideoPlayer_ffmpeg_kit->controler->comfirm_shear_btn->hide();

	m_VideoPlayer_ffmpeg_kit->slider->Finish_cutout_video();

	m_collector->stop();
	m_collector->set_playrange(m_playbeginms, m_playendms);

	int w = m_collector->get_width();
	int h = m_collector->get_height();
	double video_fps = m_collector->get_fps();
	bool open_succ = m_writer.open(QFileInfo(g_shear_videopath).absoluteFilePath().toStdString().c_str(),
		CV_FOURCC('M', 'J', 'P', 'G'),
		video_fps,
		cv::Size(w, h), false
		);

	qDebug() << QFileInfo(g_shear_videopath).absoluteFilePath().toStdString().c_str();
	qDebug() << open_succ;

	m_shear_schedule = 0;
	int framenums = m_collector->get_playrange_framenums();
	PromptBoxInst()->progbar_prepare(0, framenums, Prompt_progbar_type::Normal_Progbar, true);

	m_collector->set_runmode(VideoPlayer_ffmpeg_FrameCollector::RunMode::ShearVideoMode);
	m_collector->play();
	m_collector->start();
}
void VideoPlayer_ffmpeg::slot_shear_collect_one_frame(cv::Mat image)
{
	QTime shear_time(0, 0, 0, 0);
	shear_time = shear_time.addMSecs(m_playendms - m_playbeginms);

	QString shear_tips = QStringLiteral("剪辑时长为");
	shear_tips += shear_time.toString(QStringLiteral("hh小时mm分ss秒"));
	shear_tips += '\n';
	shear_tips += QStringLiteral("剪辑中...");

	PromptBoxInst()->progbar_go_multithread(++m_shear_schedule, shear_tips);
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
