#include "VideoPlayer_ffmpeg.h"

VideoFrameCollector_ffmpeg::VideoFrameCollector_ffmpeg(QWidget* p):
QThread(p)
{
	
}

void VideoFrameCollector_ffmpeg::set_videoname(const QString& videoname)
{
	QFileInfo to_abs_filepath(videoname);		// ��ȫ��� ���۴���������ʲô������Ϊ����·��
	m_videoname = to_abs_filepath.absoluteFilePath();
}

void VideoFrameCollector_ffmpeg::run()
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

	int get_picture = 0;
	int num = 0;
	while (true)
	{
		// ��һ֡ <0 ��˵��������
		if (av_read_frame(format_context, read_packct) < 0)
		{
			break;
		}

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

		emit collect_one_frame(out_buffer, video_codeccontext->width, video_codeccontext->height);
	}

	av_free(out_buffer);						// ��ջ�����
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
