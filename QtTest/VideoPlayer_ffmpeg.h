#pragma once

#include <windows.h>

#include <QLoggingCategory>
#include <QFile>
#include <QFileInfo>
#include <QThread>
#include <QImage>
#include <QPixmap>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QComboBox>
#include <QGraphicsEllipseItem>
#include <QStackedWidget>
#include <QMouseEvent>
#include <QApplication>
#include <QDesktopWidget>
#include <QFileInfo>
#include <QKeyEvent>
#include <QTime>
#include <QPainter>
#include <QMetaType>
#include <QMutex>

#include "PromptBox.h"
#include "externalFile/opencv/include/opencv2/core/core.hpp"
#include "externalFile/opencv/include/opencv2/core/mat.hpp"
#include "externalFile/opencv/include/opencv2/imgproc/imgproc.hpp"
#include "externalFile/opencv/include/opencv2/opencv.hpp"

#include "VidSlider.h"

extern "C"
{
#include "externalFile/ffmpeg/include/libavcodec/avcodec.h"
#include "externalFile/ffmpeg/include/libavformat/avformat.h"
#include "externalFile/ffmpeg/include/libswscale/swscale.h"
#include "externalFile/ffmpeg/include/libavdevice/avdevice.h"
#include "externalFile/ffmpeg/include/libavutil/pixfmt.h"
}

using SwsContext = struct SwsContext;
// ��Ƶ����(��λ:����)
using videoduration_ms = int64_t;

/*
@brief	ͨ����Ƶ����ͼ���ſؼ����
@author lzx
@date	20200501
@note
��Ҫ��
VideoPlayer_ffmpeg_Controler_kit	(�����������������ؼ� parent : VideoPlayer_ffmpeg_Controler)
VideoPlayer_ffmpeg_Controler		(������������� parent : VideoPlayer_ffmpeg)

VideoPlayer_ffmpeg_FrameProcesser	(ͼ�����߳� ���š���ʽת�����������������ʾ parent : VideoPlayer_ffmpeg)
VideoPlayer_ffmpeg_FrameCollector	(��Ƶ�����߳� ��ȡ��Ƶ���ԡ���ȡÿһ֡ parent : VideoPlayer_ffmpeg)
VideoPlayer_ffmpeg_kit				(�����������ؼ� parent : VideoPlayer_ffmpeg)
VideoPlayer_ffmpeg					(���������� parent : �ⲿ����)
*/
#define VideoPlayer_ffmpeg_HELPER

/*
@brief
��������
@note
�����źŴ���
VideoPlayer_ffmpeg_FrameCollector(��Ƶ�����߳�)		VideoPlayer_ffmpeg_Controler(������� ���١���ͣ�����š�������)
|													[�����ؼ� VideoPlayer_ffmpeg_Controler_kit]
|													|
|													|
VideoPlayer_ffmpeg(������)----------------------------
[�����ؼ� VideoPlayer_ffmpeg_kit]
*/
#define VideoPlayer_ffmpeg_PROCESS_ORDER

class VideoPlayer_ffmpeg;

/*
@brief
�����ݴ����� ���ڻ�ȡ��Ƶ �Լ� ����Ƶ��ȡͼ�� ��ת��ͼ���ʽ
@note
1��ʹ��ffmpeg����Ƶ�ָ����Ƶ��Ƶ�� ����֡����ͨ���źŷ����ȥ
2��important!!! ÿ�β������ �� �رղ��Ŵ��� ��������ζ�ű��̱߳��ս�!!!
*/
class VideoPlayer_ffmpeg_FrameCollector : public QThread
{
	Q_OBJECT

public:
	explicit VideoPlayer_ffmpeg_FrameCollector(VideoPlayer_ffmpeg* p = nullptr);
	~VideoPlayer_ffmpeg_FrameCollector(){}

	/*!
	�̲߳ɼ�ģʽ
	@param[1]	ShowVideoMode		��ͨģʽ �ɼ�һ֡�������ʾ
	@param[1]	ShearVideoMode		����ģʽ �ɼ�һ֡����б���
	*/
	typedef enum class RunMode
	{
		ShowVideoMode = 0,
		ShearVideoMode,
	}
	RunMode;

public:
	/*
	@brief
	�����������Ƶ·�� ��ʼ��ffmpeg���л���
	@param[1] video_path	�������Ƶ·��			QString
	@param[2] show_size		ͼ�����ŵ���ʾ�Ĵ�С		QSize
	@param[3] playspeed		�����ٶ� Ĭ��Ϊ1			int
	@return ����ֵ int
	<0		��ʼ��ʧ��
	other	��Ƶ��ʱ��(��λ:����)
	*/
	videoduration_ms init(const QString& video_path);

	void set_showsize(QSize show_size){ m_showsize = show_size; }

	/*
	@brief!
	���ű���

	@note
	���Ӳ��ű��� ����ζ�����Ӳ���֡��
	*********************************************************
	����
	60fps����Ƶ2���ٲ��� ����ζ�Ų���֡��Ӧ��������120fps
	60fps����Ƶ0.5���ٲ��� �򲥷�֡��ӦΪ30fps
	*********************************************************
	�ı䲥��֡��
	��ֱ�۵Ĵ���ʽ���Ǹı������ĵ�֡���۴���ʱ��
	���ٻ������߳�����ʱ��
	*********************************************************

	@example
	������2����		->	��֡���۴���ʱ����� / 2
	������0.5����	->	��֡���۴���ʱ��ӱ� / 0.5
	*/
	void set_playspeed(double playspeed){ m_process_perframe = m_process_perframe_1speed / playspeed; }

	/*!
	@brief
	���ò��ŷ�Χ
	@param
	[1] beginms	
	*/
	void set_playrange(videoduration_ms beginms, videoduration_ms endms)
	{
		m_video_playbegin = beginms;
		m_video_playend = endms;
		leap(m_video_playbegin);
	}

	void set_runmode(RunMode mode){ m_runmode = mode; }

	/*
	@brief!
	�鿴�Ƿ񲥷����
	@note
	�ж����� : �߳�û���������� �� ����״̬Ϊ��̬
	*/
	bool is_end(){ return (m_play_status == play_status::STATIC && !m_is_thread_run); }

	/*
	@brief
	�Ƿ����ڲ���
	@note
	�ж����� : �̲߳�����ͣ״̬ �� ����״̬Ϊ��̬
	*/
	bool is_playing(){ return (m_play_status == play_status::DYNAMIC && !m_is_thread_pausing); }

	/*
	@brief
	�Ƿ�������ͣ
	@note
	�ж����� : �߳�����ͣ״̬ �� ����״̬Ϊ��̬
	*/
	bool is_pause(){ return (m_play_status == play_status::STATIC && m_is_thread_pausing); }

	int get_fps(){ return m_fps; }
	int get_width(){ return m_video_codeccontext->width; }
	int get_height(){ return m_video_codeccontext->height; };

public slots:
	/*
	@brief
	����
	@param[1] begin		��ʼλ�� Ĭ��Ϊ��ͷ(0)��ʼ		int
	@param[2] finish	����λ�� Ĭ��Ϊ�����Ϊֹ(-1)		int
	*/
	void play(int begin = 0, int finish = -1);

	void pause(){ m_play_status = play_status::STATIC; while (!m_is_thread_pausing); }

	void stop(){ m_is_thread_run = false; this->wait(); while (this->isRunning()); }

	/*
	@brief!
	��Ƶ��ת����
	@param[1]	��Ƶ����ʱ��(��λ:����)
	*/
	void leap(int msec);

	/*!
	@brief
	�ͷ�ʹ�õ�����Ƶ��Դ
	*/
	void relaese();

protected:
	virtual void run();

signals:
	/*
	@brief
	�ɼ���һ֡���ź�
	@param[1] image �Ѿ�ת���˸�ʽ�ʹ�С��ͼ�� ��������ʾ QImage
	*/
	void show_one_frame(const cv::Mat image);
	/*
	@brief
	�ɼ�ֹͣ���ź� ��ʾ��Ƶ���Ž���
	*/
	void finish_collect_frame();

	void shear_one_frame(const cv::Mat image);
	void finish_shear();

	/*
	@brief
	������ +1000ms ����ʱ��
	@param[1] msec	���Ž���(��λ:����)
	*/
	void playtime_changed(int msec);

private:
	AVFormatContext* m_format_context;			// ��Ƶ����
	AVCodecContext* m_video_codeccontext;		// ����������
	AVStream* m_videostream;					// ��Ƶ��
	AVCodec* m_video_codec;						// ��Ƶ������
	AVFrame* m_orifmt_frame;					// ���ڴ���Ƶ��ȡ��ԭʼ֡�ṹ
	AVFrame* m_swsfmt_frame;					// ���ڽ� ԭʼ֡�ṹ ת������ʾ��ʽ�� ֡�ṹ
	SwsContext* m_img_convert_ctx;				// ����ת��ͼ���ʽ��ת����
	uint8_t* m_frame_buffer;					// ֡������
	AVPacket* m_read_packct;					// ���ڶ�ȡ֡�����ݰ�

	QImage::Format m_image_fmt;					// ��ʾ��ͼ���ʽRGB24
	QSize m_showsize;							// ��ʾ���ڴ�С
	int m_videostream_idx;						// ��Ƶ��λ��
	int m_framenum;								// ��֡��
	int m_fps;									// ��Ƶ֡��
	double m_second_timebase;					// ���ڼ���(��)����ʱ���
	/*
	@brief
	1���������
	������Ƶ֡�ʼ������
	������ÿ֡����������Ҫ��ʱ��(��λ:ms)
	@attention
	ֻ�ڳ�ʼ����Ƶʱ����һ�μ��㸳ֵ
	�����й����б��벻�ɸı�
	@see
	���崦�����Ϊ
	ffmpeg����һ֡AVFrame ->
	��Ƶ��ʽת��AVFrame -> 
	���ݸ�ʽת��cv::Mat -> 
	����cv::Mat -> 
	���ݸ�ʽת��QImage -> 
	�����ź�
	@note
	���ڼ����߳���һ��ѭ��֮�� �߳���Ҫ���ߵ�ʱ�䳤��
	*/
	double m_process_perframe_1speed;
	/*
	@brief!
	Ӧ�����߳��еĵ�֡����ʱ��
	���й����пɸı�
	*/
	double m_process_perframe;	
	videoduration_ms m_video_msecond;			// ��Ƶ������
	videoduration_ms m_video_playbegin;			// ��Ƶ���ſ�ʼλ��
	videoduration_ms m_video_playend;			// ��Ƶ���Ž���λ��

	bool m_is_thread_run;						// �߳̿���λ
	bool m_is_thread_pausing;					// �߳���ͣ�ж�λ
	bool m_is_init;

	/*
	@brief
	����������״̬
	@param[1] STATIC	���洦�ھ�ֹ״̬ ��ͣ��δ���Ŷ�ʹ�����״̬
	@param[2] DYNAMIC	���洦�ڶ�̬ ����ʱʹ�����״̬
	*/
	typedef enum class play_status
	{
		// @param[1] STATIC	���洦�ھ�ֹ״̬ ��ͣ��δ���Ŷ�ʹ�����״̬
		STATIC = 0,
		// @param[2] DYNAMIC	���洦�ڶ�̬ ����ʱʹ�����״̬
		DYNAMIC = !STATIC
	}
	play_status;
	play_status m_play_status;

	RunMode m_runmode;
};

/*
@brief
��Ƶ�����������ؼ�
*/
typedef struct VideoPlayer_ffmpeg_ControlPanel_kit : public QWidget
{
	Q_OBJECT

public:
	explicit VideoPlayer_ffmpeg_ControlPanel_kit(QLabel* p);
	~VideoPlayer_ffmpeg_ControlPanel_kit(){}

public:
	QPushButton* play_or_pause_btn;							// ������ͣ��ť
	QLabel* video_playtime;									// ��Ƶ����ʱ��
	QLabel* video_sumtime;									// ��Ƶ��ʱ��
	QComboBox* playspeed_box;								// ����ѡ��

	QStackedWidget* func_button_list;						// ���ܼ��б�
	QPushButton* delete_btn;								// ɾ����Ƶ
	QPushButton* shear_btn;									// ������Ƶ

	QPushButton* comfirm_shear_btn;							// ȷ�ϼ�����Ƶ
	QPushButton* cancle_shear_btn;							// ȡ��������Ƶ
}
VideoPlayer_ffmpeg_Controler_kit;

/*
@brief
��Ƶ���������
*/
typedef struct VideoPlayer_ffmpeg_kit
{
public:
	explicit VideoPlayer_ffmpeg_kit(VideoPlayer_ffmpeg* p = nullptr);

public:
	QLabel* frame_displayer;								// ֡��ʾ��
	QWidget* slider_background;								// ���Ž���������
	VidSlider* slider;										// ���Ž�����
	VideoPlayer_ffmpeg_ControlPanel_kit* controler;			// �������
}
VideoPlayer_ffmpeg_kit;

/*
@brief
��Ƶ���������� �������ݴ���������ȡ֡���ݲ�����
@note
Ĭ������
*/
class VideoPlayer_ffmpeg :public QWidget
{
	Q_OBJECT

public:
	/*
	@brief
	��ʾ״̬
	@param[1] VideoStyle				��Ƶ��ʽ
	@param[2] GifStyle_without_slider	�޽������Ķ�ͼ��ʽ
	@param[3] GifStyle_with_slider		���������Ķ�ͼ��ʽ
	*/
	enum class video_or_gif
	{
		VideoStyle = 0,
		GifStyle_without_slider,
		GifStyle_with_slider,
	};

	/*
	@brief
	ý������״̬
	@param[1] Succeed				�ɹ�
	@param[2] NoVideoMessage		û���ҵ���Ƶ��Ϣ
	@param[3] VideoAnalysisFailed	��Ƶ��������
	*/
	enum class set_media_status
	{
		Succeed = 0,
		NoVideoMessage,
		VideoAnalysisFailed
	};

public:
	explicit VideoPlayer_ffmpeg(QWidget* p = nullptr);
	~VideoPlayer_ffmpeg(){}

public slots:
	/*
	@brief
	������Ƶ·�� ���ò���ý��
	Ĭ��Ϊ��Ƶ��ʾģʽ(���������пؼ� �� ���ŷ�ΧΪ��ͷ��β)
	@param[1] video_path	��Ƶ���·�������·��							QString
	@param[2] sta			��������ʽ ��Ƶ ��ͼ(�ַ��н�������û������)		video_or_gif
	@param[3] begin			��Ƶ���ŷ�Χ��ͷ(��λ:����) Ĭ�ϴ�ͷ��ʼ ��0		int
	@param[4] finish		��Ƶ���ŷ�Χ��β(��λ:����) Ĭ�ϴ�ͷ��ʼ ��-1		int
	@return!
	����ֵ
	true	���óɹ�
	false	����ʧ��
	@note!
	ֻ��Ҫ��������Ƶ·����ʱ�����һ�� ����·��������ظ�����
	*/
	set_media_status set_media(const QString & video_path,
		video_or_gif sta = video_or_gif::VideoStyle,
		videoduration_ms beginms = 0,
		videoduration_ms endms = -1);
	void set_style(video_or_gif sta = video_or_gif::VideoStyle);
	/*
	@brief
	������Ƶ���ŷ�Χ(��λ:����)
	@attention
	���ŷ�Χ �� ���������Χû��ֱ�ӹ���
	*/
	void set_playrange(videoduration_ms beginms, videoduration_ms endms);
	/*
	@brief
	������Ƶ���ſ�ʼʱ��(��λ:����)
	*/
	void set_playbegin(videoduration_ms beingms);
	/*
	@brief
	������Ƶ���Ž���ʱ��(��λ:����)
	*/
	void set_playend(videoduration_ms finishms);
	/*
	@brief
	����Ĭ�ϲ��ű���
	@attention
	��Ƶ����ͼ����Ĭ�϶�Ϊ1����
	Ԥ���ӿ�
	��������²�Ӧ�ñ����� 
	����֮��ͼ��Ҫ���ٲ���
	������Զ�����õ�
	*/
	void set_playspeed(int playspeed);
	/*
	@brief
	gif����ר�� �����Ƿ��ظ�����
	@note
	������������󽫻����Ͽ�ʼ������Ƶ
	*/
	void set_replay(bool replay);

protected:
	/*
	@brief
	�����¼�
	@note
	��ʾ�������
	*/
	void enterEvent(QEvent* event);
	/*
	@brief
	�˳��¼�
	@ntoe
	���ؿ������
	*/
	void leaveEvent(QEvent* event);
	/*
	@brief
	�����¼�
	@note
	1��ctrl + w �رղ�����
	*/
	void keyPressEvent(QKeyEvent* event);
	/*
	@brief
	�¼�������
	@note
	1���ڲ�����QSlider������� ʵ�ֲ��Ž������洦�ɵ�Ĺ���
	@see
	ʹ���¼������� ��Ҫ�ڹ��캯������son->installEventFilter(par)���� ��ע�������
	*/
	bool eventFilter(QObject* watched, QEvent* event);

private:
	void clear_videodisplayer();
	void init_playspeed_box();

private slots:
	/*
	@brief
	��ʾһ֡
	*/
	void slot_show_collect_one_frame(const cv::Mat image);
	void slot_finish_show_frame();

	/*
	@brief
	��������ת(��λ:ms)
	*/
	void slot_playtime_changed(int msec);

	/*
	@brief
	������ͣ
	*/
	void slot_play_or_pause();
	/*
	@brief
	��Ӧ�������¼�������
	*/
	void slot_slider_triggered();

	void slot_playspeed_changed(int box_idx);

	void slot_delete_video();
	/*!
	@brief
	������Ƶ׼��״̬
	*/
	void slot_ready_shear_video();
	/*!
	@brief
	ȡ��������Ƶ
	*/
	void slot_cancle_shear_video();
	/*!
	@brief
	ȷ�ϼ�����Ƶ
	*/
	void slot_confirm_shear_video();
	void slot_shear_collect_one_frame(cv::Mat image);
	void slot_shear_finish();

	void slot_exit();

signals:
	void play_finish();

private:	
	VideoPlayer_ffmpeg_kit* m_VideoPlayer_ffmpeg_kit;

	VideoPlayer_ffmpeg_FrameCollector* m_collector;

	QString m_abs_videopath;

	bool m_video_exist;
	bool m_controller_always_hide;
	bool m_replay_after_finish;

	const std::vector<double> m_playspeed_choosen;	// ��ѡ�Ĳ��ű���
	const int m_playspeed_1_index;					// 1���ٵ�����λ��

	videoduration_ms m_playbeginms;
	videoduration_ms m_playendms;

	cv::VideoWriter m_writer;
};
