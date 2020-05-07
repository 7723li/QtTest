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

#include <thread>

#include "externalFile/opencv/include/opencv2/opencv.hpp"

#include "PromptBox.h"
#include "VideoListWidget.h"
#include "AVTCamera.h"
#include "VideoPlayer_ffmpeg.h"
#include "VideoDisplayWidget.hpp"


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
	VideoDisplayWidget* video_displayer;// ��Ƶ��ʾ��
	QLabel* bed_num_label;				// ����
	QLabel* patient_name_label;			// ����
	QLabel* video_time_display;			// ��Ƶʱ����ʾ
	QPushButton* record_btn;			// (ֹͣ)¼�ư�ť
	QPushButton* exit_btn;				// ������鰴ť

	QWidget* video_list_background;		// ��Ƶ����б�Ĵ��ױ�����
	VideoListWidget* video_list;		// ��Ƶ����б�

	VideoPlayer_ffmpeg* videoplayer;
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

public:
	/*
	@brief
	�ⲿ�л��������
	@param[1] examid ����id QString
	*/
	void prepare_record(const QString & examid);

private:
	void clear_video_displayer();	// ����ͼ����ʾ����

private slots:
	void slot_get_one_frame();											// ���������ȡһ֡
	void slot_begin_or_finish_record();									// ��ʼ(����)¼����Ƶ
	void slot_timeout_video_duration_timer();							// ���¼�Ƽ�ʱ
	void slot_replay_recordedvideo(QListWidgetItem* choosen_video);		// �ز�¼�Ƶ���Ƶ
	void slot_exit();													// �˳�����

signals:
	void PageVideoRecord_exit(const QString & examid);

private:
	PageVideoRecord_kit* m_PageVideoRecord_kit;

	std::vector<QString> m_recored_videoname_list;	// ��ǰ����֮ǰ¼�ƹ�����Ƶ ��������¼����ɵ���Ƶ

	QString m_examid;					// ����ID ���ⲿ����

	QTimer* m_get_frame_timer;			// ���ڴ������ȡ֡���ݵĶ�ʱ��
	QTimer* m_record_duration_timer;	// ���ڼ�¼¼��ʱ���Ķ�ʱ��
	int m_record_duration_period;		// ¼��ʱ����ʱ��������� = ¼��ʱ������

	cv::Mat m_mat;						// cv::Mat->QImage->QPixmap->QLabel����
	cv::VideoWriter m_VideoWriter;		// ��Matд�뵽��Ƶ

	camerabase* m_camerabase;			// �����װ ͨ������ӿ�
	AVTCamera* m_avt_camera;			// AVT���
};
