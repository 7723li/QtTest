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
	QFileInfo to_abs_filepath(videoname);		// 安全起见 无论传进来的是什么都先设为绝对路径
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
	av_register_all();	// 初始化ffmpeg环境

	// 格式内容 贯穿整个ffmpeg生命周期 大概就是一个视频文件的属性吧
	AVFormatContext* format_context = avformat_alloc_context();

	// 判断视频在不在 打开一个格式内容 其实就是打开一个视频 并右键"属性"
	const char* file_name = m_videoname.toStdString().c_str();
	if (!QFile::exists(m_videoname) || avformat_open_input(&format_context, m_videoname.toStdString().c_str(), nullptr, nullptr) < 0)
	{
		return;
	}

	// 根据 格式内容 寻找视频流、音频流之类的信息(TopV项目的视频一般都只有视频流)
	if (avformat_find_stream_info(format_context, nullptr) < 0)
	{
		return;
	}

	// 寻找视频流的位置
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

	// 视频解码器内容 如码所见 就是找到的视频流的解码器内容
	AVCodecContext* video_codeccontext = format_context->streams[video_stream_index]->codec;

	// ffmpeg提供了一系列的解码器 这里就是根据id找要用的那个解码器
	AVCodec* video_codec = avcodec_find_decoder(video_codeccontext->codec_id);
	if (nullptr == video_codec)
	{
		return;
	}

	// 打开解码器
	if (avcodec_open2(video_codeccontext, video_codec, nullptr) < 0)
	{
		return;
	}

	// 申请两个帧结构 一个视频帧的默认格式(一般为YUV420) 一个转存图像的格式(默认为RGB)
	AVFrame* frame = av_frame_alloc();
	AVFrame* frameRGB = av_frame_alloc();

	// one frame scale to another 需要的转码内容
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

	// (转存格式)一帧需要的字节数 取决于 格式、图像大小
	int num_bytes = avpicture_get_size(AV_PIX_FMT_GRAY8, video_codeccontext->width, video_codeccontext->height);
	

	// 根据(转存格式)一帧需要的字节数 给帧申请一个缓冲区 来装载一帧的内容
	uint8_t* out_buffer = (uint8_t*)av_malloc(num_bytes * sizeof(uint8_t));
	avpicture_fill((AVPicture*)frameRGB, out_buffer, AV_PIX_FMT_GRAY8, video_codeccontext->width, video_codeccontext->height);

	// 图像大小
	int frame_size = video_codeccontext->width * video_codeccontext->height;

	// 分配一个数据包 读取一(视频)帧之后 用来存(视频)帧的数据
	AVPacket* read_packct = new AVPacket;
	av_new_packet(read_packct, frame_size);

	// 打印输入视频的格式
	// av_dump_format(format_context, 0, file_name, 0);

	QPixmap pixmap(video_codeccontext->width, video_codeccontext->height);
	int get_picture = 0;
	int pic_area = video_codeccontext->width * video_codeccontext->height;
	while (true)
	{
		// 读一帧 <0 就说明读完了
		if (av_read_frame(format_context, read_packct) < 0)
		{
			break;
		}

		read_packct->pts;

		// 安全性判断 确保是视频流出来的东西
		if (read_packct->stream_index == video_stream_index)
		{
			// 解码一帧
			int ret = avcodec_decode_video2(video_codeccontext, frame, &get_picture, read_packct);

			if (ret < 0)
				break;
		}

		if (get_picture)
		{
			// 帧图像格式转换
			sws_scale(img_convert_ctx, frame->data, frame->linesize, 0, video_codeccontext->height, frameRGB->data, frameRGB->linesize);
		}

		QImage& image = QImage(out_buffer, video_codeccontext->width, video_codeccontext->height, QImage::Format_Grayscale8);
		QPixmap& pixmap = QPixmap::fromImage(image);
		pixmap = pixmap.scaled(m_showsize);
		emit collect_one_frame(pixmap, 0);
	}

	av_free(out_buffer);						// 清空缓冲区
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
