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

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>
#include <QTime>
#include <QTimer>

#include "PromptBox.h"
#include "VideoListWidget.h"

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
	QLabel* video_displayer;			// ��Ƶ��ʾ��
	QLabel* bed_num_label;				// ����
	QLabel* patient_name_label;			// ����
	QLabel* video_duration_display;		// ��Ƶʱ����ʾ
	QPushButton* record_btn;			// (ֹͣ)¼�ư�ť
	QPushButton* finish_check_btn;		// ������鰴ť

	QWidget* video_list_background;		// ��Ƶ����б�Ĵ��ױ�����
	VideoListWidget* video_list;		// ��Ƶ����б�
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

private slots:
	void slot_video_recorded();

private:
	PageVideoRecord_kit* m_PageVideoRecord_kit;

	QTimer* m_video_duration_timer;	// ¼��ʱ����¼��ʱ��
	int m_video_duration_realtime;	// ��ʵ��¼��ʱ�� = ¼��ʱ����¼��ʱ�� timeout����
};
