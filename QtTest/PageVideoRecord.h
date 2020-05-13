#pragma once

/*
@author : lzx
@date	: 20200428
@brief
¼�ƽ���
@property
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

#include <thread>

#include "externalFile/opencv/include/opencv2/opencv.hpp"

#include "PromptBox.h"
#include "VideoListWidget.h"
#include "AVTCamera.h"
#include "VideoPlayer_ffmpeg.h"

/*
@brief
¼�ƽ��湤����
*/
typedef struct PageVideoRecord_kit
{
public:
	explicit PageVideoRecord_kit(QWidget* p);
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
	void enter_PageVideoRecord(const QString & examid);		// ����¼�ƽ���
	/*
	@brief
	�ⲿ�˳�¼�ƽ���ӿ�
	*/
	void exit_PageVideoRecord();							// �˳�¼�ƽ���

private:
	void clear_videodisplay();								// ����ͼ����ʾ����
	
	void load_old_videos();									// ����֮ǰ��¼�� ����Ƶ
	
	void put_video_thumb(const QString & video_path, const QString & icon_path);	// ����һ����Ƶ����ͼ

	bool open_camera();										// �����
	void close_camera();									// �ر����

	void show_camera_openstatus(int openstatus);			// ��ʾ�����״̬
	
	void begin_capture();									// ��ʼ��׽Ӱ��(�������߳�)
	void stop_capture();									// ֹͣ��׽Ӱ��(�ı�״̬ �ر����߳�)
	void capture_thread();									// ���߳� ������Ļ���������ȡ�µ�һ֡

	bool get_useful_fps(double & fps);						// ��ȡһ����Ч(>0)��֡�� ������ʾ��¼��

	void begin_show_frame();								// ��ʼ��ʾͼ��(������ʱ�� ���м������֡�ʾ���)
	void stop_show_frame();									// ֹͣ��ʾͼ��(�رն�ʱ��)

	void begin_record();									// ��ʼ¼��
	void stop_record();										// ֹͣ¼��(�ı�״̬ ������Ƶ)

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

	std::vector<QString> m_recored_videoname_list;			// ��ǰ����֮ǰ¼�ƹ�����Ƶ ��������¼����ɵ���Ƶ

	QString m_examid;										// ����ID ���ⲿ����

	QTimer* m_show_frame_timer;								// ��ʾͼ��ʱ��
	QTimer* m_record_duration_timer;						// ���ڼ�¼¼��ʱ���Ķ�ʱ��
	int m_record_duration_period;							// ¼��ʱ����ʱ��������� = ¼��ʱ������

	cv::Mat m_mat;											// ֡���ݻ���
	cv::VideoWriter m_VideoWriter;							// ��Matд�뵽��Ƶ

	camerabase* m_camerabase;								// �����װ ͨ������ӿ�
	AVTCamera* m_avt_camera;								// AVT���

	bool m_is_need_capture;									// �Ƿ���Ҫ(����)��׽Ӱ��
	bool m_is_capturing;									// �Ƿ����ڲ�׽Ӱ��

	bool m_is_need_record;									// �Ƿ���Ҫ(����)¼����Ƶ
	bool m_is_recording;									// �Ƿ�����¼����Ƶ
};
