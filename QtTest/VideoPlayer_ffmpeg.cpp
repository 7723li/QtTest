#include "VideoPlayer_ffmpeg.h"

VideoPlayer_ffmpeg_FrameProcesser::VideoPlayer_ffmpeg_FrameProcesser(VideoPlayer_ffmpeg* p)
{

}







VideoPlayer_ffmpeg_FrameCollector::VideoPlayer_ffmpeg_FrameCollector(VideoPlayer_ffmpeg* p) : 
QThread(p)
{
	m_playspeed = 1;
	m_videostream_idx = -1;

	m_is_run = false;
}

void VideoPlayer_ffmpeg_FrameCollector::set_param(
	AVCodecContext* video_codeccontext,
	AVFormatContext* format_context,
	AVPacket* read_packct, 
	AVFrame* oriframe, 
	AVFrame* swsframe,
	SwsContext* img_convert_ctx,
	uint8_t* frame_buffer, 
	QSize showsize, 
	QImage::Format img_fmt,
	int videostream_idx,
	int playspeed)
{
	m_video_codeccontext = video_codeccontext;
	m_format_context = format_context;
	m_read_packct = read_packct;
	m_oriframe = oriframe;
	m_swsframe = swsframe ;
	m_img_convert_ctx = img_convert_ctx;
	m_frame_buffer = frame_buffer;
	m_showsize = showsize;
	m_img_fmt = img_fmt;
	m_playspeed = playspeed;
	m_videostream_idx = videostream_idx;

	m_is_run = true;
}
void VideoPlayer_ffmpeg_FrameCollector::pause()
{

}
void VideoPlayer_ffmpeg_FrameCollector::stop()
{

}
void VideoPlayer_ffmpeg_FrameCollector::run()
{
	QPixmap pixmap(m_video_codeccontext->width, m_video_codeccontext->height);
	int get_picture = 0;
	while (true)
	{
		if (!m_is_run)
		{
			break;
		}

		get_picture = 0;

		// 读一帧 <0 就说明读完了
		if (av_read_frame(m_format_context, m_read_packct) < 0)
		{
			break;
		}

		int pts = m_read_packct->pts;

		// 安全判断 确保解码的是视频流
		if (m_read_packct->stream_index != m_videostream_idx)
			continue;

		// 解码一帧
		int ret = avcodec_decode_video2(m_video_codeccontext, m_oriframe, &get_picture, m_read_packct);
		// 如果 解码成功 并且 该帧为关键帧 才去转码
		if (ret > 0 && get_picture > 0)
		{
			// 转码一帧
			sws_scale(m_img_convert_ctx, m_oriframe->data, m_oriframe->linesize, 0, m_video_codeccontext->height, m_swsframe->data, m_swsframe->linesize);
		}
		
		// 保持显示
		QImage& image = QImage(m_frame_buffer, m_video_codeccontext->width, m_video_codeccontext->height, m_img_fmt);
		QPixmap& pixmap = QPixmap::fromImage(image);
		pixmap = pixmap.scaled(m_showsize);
		emit collect_one_frame(pixmap, 0);
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

	m_format_context = nullptr;
	m_video_codeccontext = nullptr;
	m_video_codec = nullptr;
	m_oriframe = nullptr;
	m_swsframe = nullptr;
	m_img_convert_ctx = nullptr;
	m_frame_buffer = nullptr;
	m_read_packct = nullptr;

	m_collector = new VideoPlayer_ffmpeg_FrameCollector(this);
	connect(m_collector, &VideoPlayer_ffmpeg_FrameCollector::collect_one_frame, this, &VideoPlayer_ffmpeg::slot_show_one_frame);
	connect(m_collector, &VideoPlayer_ffmpeg_FrameCollector::finish_collect_frame, this, &VideoPlayer_ffmpeg::slot_finish_collect_frame);

	m_controller_always_hide = false;
}

VideoPlayer_ffmpeg::~VideoPlayer_ffmpeg()
{

}

void VideoPlayer_ffmpeg::play(const QString & video_path, video_or_gif sta, int playspeed)
{
	if (sta == video_or_gif::VideoStyle)
		m_controller_always_hide = false;
	else if (sta == video_or_gif::GifStyle)
		m_controller_always_hide = true;

	QFileInfo video_info(video_path);
	if (!video_info.exists())
	{
		PromptBoxInst(this)->msgbox_go(PromptBox_msgtype::Warning,
			PromptBox_btntype::None,
			QStringLiteral("没有找到视频信息"),
			1000, true);
		return;
	}

	m_video_path = video_info.absoluteFilePath();
	init_ffmpeg();
	bool analy_succ = analysis_video();
	if (!analy_succ)
	{
		PromptBoxInst(this)->msgbox_go(PromptBox_msgtype::Warning,
			PromptBox_btntype::None,
			QStringLiteral("视频解析错误"),
			1000, true);
		return;
	}

	m_play_speed = playspeed;

	m_collector->set_param(m_video_codeccontext,
		m_format_context,
		m_read_packct,
		m_oriframe,
		m_swsframe,
		m_img_convert_ctx,
		m_frame_buffer,
		m_VideoPlayer_ffmpeg_kit->frame_displayer->size(),
		m_image_fmt,
		m_play_speed,
		m_videostream_idx
		);
	m_collector->start();
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

void VideoPlayer_ffmpeg::init_ffmpeg()
{
	av_register_all();	// 初始化ffmpeg环境
}
bool VideoPlayer_ffmpeg::analysis_video()
{
	// 打开一个格式内容 并右键查看"视频属性"
	m_format_context = avformat_alloc_context();
	int open_format = avformat_open_input(&m_format_context, m_video_path.toStdString().c_str(), nullptr, nullptr);
	if (open_format < 0)
	{
		return false;
	}

	// 根据 格式内容 寻找视频流、音频流之类的信息(TopV项目的视频一般都只有视频流)
	if (avformat_find_stream_info(m_format_context, nullptr) < 0)
	{
		return false;
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
		return false;
	}

	// 找到视频流的解码器内容
	m_video_codeccontext = m_format_context->streams[m_videostream_idx]->codec;
	if (nullptr == m_video_codeccontext)
	{
		return false;
	}

	// ffmpeg提供了一系列的解码器 这里就是根据id找要用的那个解码器
	m_video_codec = avcodec_find_decoder(m_video_codeccontext->codec_id);
	if (nullptr == m_video_codec)
	{
		return false;
	}

	// 打开解码器
	if (avcodec_open2(m_video_codeccontext, m_video_codec, nullptr) < 0)
	{
		return false;
	}

	// 申请帧结构
	m_oriframe = av_frame_alloc();
	if (nullptr == m_oriframe)
	{
		return false;
	}
	m_swsframe = av_frame_alloc();
	if (nullptr == m_swsframe)
	{
		return false;
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
		return false;
	}
	m_image_fmt = QImage::Format_Grayscale8;

	// 根据转存格式计算 一帧需要的字节数 取决于 格式、图像大小
	int num_bytes = avpicture_get_size(
		AV_PIX_FMT_GRAY8,
		m_video_codeccontext->width,
		m_video_codeccontext->height);
	if (num_bytes <= 0)
	{
		return false;
	}

	// 根据一帧需要的字节数 给帧申请一个缓冲区 来装载一帧的内容
	m_frame_buffer = (uint8_t*)av_malloc(num_bytes * sizeof(uint8_t));
	if (nullptr == m_frame_buffer)
	{
		return false;
	}

	int fill_succ = avpicture_fill((AVPicture*)m_swsframe, m_frame_buffer, AV_PIX_FMT_GRAY8,
		m_video_codeccontext->width, m_video_codeccontext->height);
	if (fill_succ < 0)
	{
		return false;
	}

	// 图像面积
	int frame_area = m_video_codeccontext->width * m_video_codeccontext->height;

	// 分配一个数据包 读取一(视频)帧之后 用来存(视频)帧的数据
	m_read_packct = new AVPacket;
	if (av_new_packet(m_read_packct, frame_area) < 0)
	{
		return false;
	}

	return true;
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
	m_collector->stop();

	av_free(m_frame_buffer);						// 清空缓冲区
	av_free(m_oriframe);							// 清空原始帧格式
	av_free(m_swsframe);							// 清空转换帧格式
	av_free_packet(m_read_packct);
	avcodec_close(m_video_codeccontext);			// 关闭解码器
	avformat_close_input(&m_format_context);		// 关闭视频属性

	if (nullptr != m_read_packct)
		delete m_read_packct;
	m_read_packct = nullptr;

	emit play_finish();
}
