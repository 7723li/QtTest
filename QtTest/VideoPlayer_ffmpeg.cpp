#include "VideoPlayer_ffmpeg.h"

VideoFrameCollector_ffmpeg::VideoFrameCollector_ffmpeg(QWidget* p):
QThread(p)
{
	
}

void VideoFrameCollector_ffmpeg::set_videoname(const QString& videoname)
{
	QFileInfo to_abs_filepath(videoname);		// 安全起见 无论传进来的是什么都先设为绝对路径
	m_videoname = to_abs_filepath.absoluteFilePath();
}

void VideoFrameCollector_ffmpeg::run()
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

	int get_picture = 0;
	int num = 0;
	while (true)
	{
		// 读一帧 <0 就说明读完了
		if (av_read_frame(format_context, read_packct) < 0)
		{
			break;
		}

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

		emit collect_one_frame(out_buffer, video_codeccontext->width, video_codeccontext->height);
	}

	av_free(out_buffer);						// 清空缓冲区
	av_free(frameRGB);
	avcodec_close(video_codeccontext);
	avformat_close_input(&format_context);
	emit finish_collect_frame();
}





VideoPlayer_ffmpeg_kit::VideoPlayer_ffmpeg_kit(QWidget* p) :
	QWidget(p)
{
	frame_displayer = new FrameDisplayWidget(this);
	frame_displayer->setGeometry(0, 0, 900, 900);
}






VideoPlayer_ffmpeg::VideoPlayer_ffmpeg(QWidget* p):
QWidget(p)
{
	m_collector = new VideoFrameCollector_ffmpeg(this);
	connect(m_collector, &VideoFrameCollector_ffmpeg::collect_one_frame, this, &VideoPlayer_ffmpeg::show_frame, Qt::DirectConnection);
	connect(m_collector, &VideoFrameCollector_ffmpeg::finish_collect_frame, this, &VideoPlayer_ffmpeg::slot_finish_collect_frame);

	m_VideoPlayer_ffmpeg_kit = new VideoPlayer_ffmpeg_kit(this);
}

VideoPlayer_ffmpeg::~VideoPlayer_ffmpeg()
{

}

void VideoPlayer_ffmpeg::play(const QString & video_name)
{
	m_VideoPlayer_ffmpeg_kit->frame_displayer->show();
	this->show();
	m_collector->set_videoname(video_name);
	m_collector->start();
}

void VideoPlayer_ffmpeg::show_frame(uchar* frame_date, int w, int h)
{
	m_VideoPlayer_ffmpeg_kit->frame_displayer->show_frame(frame_date, w, h);
}

void VideoPlayer_ffmpeg::slot_finish_collect_frame()
{
	m_collector->quit();
	m_collector->terminate();
	m_VideoPlayer_ffmpeg_kit->frame_displayer->hide();
	this->hide();

	emit finish_play_video();
}
