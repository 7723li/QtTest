#pragma once

/*
@author : lzx
@date	: 20200428
@brief
¼�ƽ���
@property
ͼ���ȡ���߳�(VideoCapture)
¼�ƽ��湤����(PageVideoCollect_kit)
¼�ƽ��汾��(PageVideoRecord)
*/

#include <ctime>

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>
#include <QTime>
#include <QTimer>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QPaintEvent>
#include <QPainter>

#include "externalFile/opencv/include/opencv2/opencv.hpp"

#include "PromptBox.h"
#include "VideoListWidget.h"
#include "AVTCamera.h"
#include "VideoPlayer_ffmpeg.h"

class PageVideoRecord;

class VideoSaver : public QThread
{
public:
	VideoSaver(PageVideoRecord* p, cv::VideoWriter* w) :
		QThread(nullptr), _w(w), _run(false)
	{

	}
	~VideoSaver(){}

	virtual void run() override;
	void stop();

private:
	cv::VideoWriter* _w;
	bool _run;
};

/*
@brief
¼�ƽ����ͼ���ȡ���߳�
������������������ȡ��������õ�֡����
���� ������Ƶ
*/
class VideoCapture : private QThread
{
public:
	/*
	@note
	�������Ϊ¼�ƽ���
	*/
	VideoCapture(PageVideoRecord* p);
	~VideoCapture(){}

	/*
	@brief
	��ʼ��ȡͼ��
	@param[1] c ¼�ƽ��浱ǰ����ʹ�õ���� camerabase*
	@param[2] m ¼�ƽ��� ->�Ѿ������<- ��֡�ռ� cv::Mat*
	@param[3] v ¼�ƽ����VideoWriter cv::VideoWriter*
	*/
	bool begin_capture(
		camerabase* c,
		cv::Mat* m,
		cv::VideoWriter* v);
	/*
	@brief
	ֹͣ��ȡͼ��
	*/
	void stop_capture();

	/*
	@brief
	���Կ�ʼ¼��
	*/
	void begin_record();
	/*
	@brief
	����ֹͣ¼��
	*/
	void stop_record();

	/*
	@brief
	ͼ���ȡ״̬(����Ƿ����ڹ���)
	@note
	��״̬Ӧ�ñ������Ӱ�쵽�Ƿ���Կ�ʼ¼��
	*/
	bool capturing();
	/*
	@brief
	ͼ��¼��״̬(�Ƿ�����¼��)
	@note
	��״̬Ӧ�ñ������Ӱ�쵽�Ƿ�����˳�¼�ƽ���
	*/
	bool recording();

private:
	virtual void run() override;

private:
	bool need_capture;
	bool need_record;

	camerabase* cam;
	cv::Mat* mat;
	cv::VideoWriter* writer;
};

/*
@brief
¼�ƽ��湤����
*/
typedef struct PageVideoRecord_kit
{
public:
	explicit PageVideoRecord_kit(PageVideoRecord* p);
	~PageVideoRecord_kit(){}

public:
	QLabel* frame_displayer;			// ֡��ʾ��
	QLabel* bed_num_label;				// ����
	QLabel* patient_name_label;			// ����
	QLabel* video_time_display;			// ��Ƶʱ����ʾ
	QPushButton* record_btn;			// (ֹͣ)¼�ư�ť
	QPushButton* exit_btn;				// ������鰴ť

	QWidget* video_list_background;		// ��Ƶ����б�Ĵ��ױ�����
	VideoListWidget* video_list;		// ��Ƶ����б�

	VideoPlayer_ffmpeg* videoplayer;	// ��Ƶ������
}
PageVideoRecord_kit;

/*
@brief
¼�ƽ��汾��
*/
class PageVideoRecord : public QWidget
{
	Q_OBJECT

public:
	explicit PageVideoRecord(QWidget* parent);
	~PageVideoRecord(){}

public slots:
	/*
	@brief
	�ⲿ����¼�ƽ���ӿ�
	@param[1] examid ����id QString
	*/
	void enter_PageVideoRecord(const QString & examid);
	/*
	@brief
	�ⲿ�˳�¼�ƽ���ӿ�
	*/
	void exit_PageVideoRecord();

private:
	void clear_videodisplay();								// ����ͼ����ʾ����
	
	void load_old_vidthumb();								// ����֮ǰ��¼������ͼ ����Ƶ
	void put_video_vidthumb(const QString & video_path, const QString & icon_path);	// ����һ����Ƶ����ͼ
	void clear_all_vidthumb();								// ����¼������ͼ�б�

	void show_camera_openstatus(int openstatus);			// ��ʾ�����״̬
	void show_camera_closestatus(int closestatus);			// ��ʾ����ر�״̬
	
	void begin_capture();									// ��ʼ��׽Ӱ��(�������߳�)
	void stop_capture();									// ֹͣ��׽Ӱ��(�ı�״̬ �ر����߳�)

	bool get_useful_fps(double & fps);						// ��ȡһ����Ч(>0)��֡�� ������ʾ��¼��

	void begin_show_frame();								// ��ʼ��ʾͼ��(������ʱ�� ���м������֡�ʾ���)
	void stop_show_frame();									// ֹͣ��ʾͼ��(�رն�ʱ��)

	void begin_record();									// ����һ��Ӱ��׽ѭ����ʼ¼��
	void stop_record();										// ����һ��Ӱ��׽ѭ��ֹͣ¼��
	bool apply_record_msg();									// ������Ƶͷ
	void release_record_msg();								// �ͷ���Ƶͷ(������Ƶ)
	void begin_show_recordtime();							// ��ʼ��ʾ¼��ʱ��
	void stop_show_recordtime();							// ֹͣ��ʾ¼��ʱ��

private slots:
	void slot_show_one_frame();								// ��ʾͼ��ۺ���

	void slot_begin_or_finish_record();						// ��ʼ(����)¼����Ƶ
	void slot_timeout_video_duration_timer();				// ��ʾ¼��ʱ��

	void slot_replay_begin(QListWidgetItem* choosen_video);	// �ز�¼�Ƶ���Ƶ
	void slot_replay_finish();								// �ز����

signals:
	void PageVideoRecord_exit(const QString & examid);		// �˳�¼�ƽ���ʱ�������ź� �����������

private:
	PageVideoRecord_kit* m_PageVideoRecord_kit;

	using thumb_name = std::map<QListWidgetItem*, QString>;	// ������ͼ�ҵ���Ƶ���Ƶ����ݽṹ
	thumb_name m_map_video_thumb_2_name;					// ������ͼ�ҵ���Ƶ����

	QString m_examid;										// ����ID ���ⲿ����

	QTimer* m_show_frame_timer;								// ��ʾͼ��ʱ��
	QTimer* m_record_duration_timer;						// ���ڼ�¼¼��ʱ���Ķ�ʱ��
	QTime m_record_duration_period;							// ¼��ʱ�� ÿ�ζ�ʱ������Լ�1

	cv::Mat m_mat;											// ֡���ݻ���
	cv::VideoWriter m_VideoWriter;							// ��Matд�뵽��Ƶ
	VideoCapture* m_video_capture;							// ��׽Ӱ������߳�

	camerabase* m_camerabase;								// �����װ ͨ������ӿ�
	AVTCamera* m_avt_camera;								// AVT���
};
