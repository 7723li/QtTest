#pragma once

/*
@author : lzx
@date	: 20200428
@brief
录制界面
@property
录制界面工具箱(PageVideoCollect_kit)
录制界面本体(PageVideoRecord)
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
录制界面工具箱
*/
typedef struct PageVideoRecord_kit
{
public:
	explicit PageVideoRecord_kit(QWidget* p);
	~PageVideoRecord_kit(){}

public:
	QLabel* video_displayer;			// 视频显示器
	QLabel* bed_num_label;				// 床号
	QLabel* patient_name_label;			// 姓名
	QLabel* video_duration_display;		// 视频时长显示
	QPushButton* record_btn;			// (停止)录制按钮
	QPushButton* finish_check_btn;		// 结束检查按钮

	QWidget* video_list_background;		// 视频存放列表的纯白背景板
	VideoListWidget* video_list;		// 视频存放列表
}
PageVideoRecord_kit;

/*
@brief
录制界面本体
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

	QTimer* m_video_duration_timer;	// 录像时长记录定时器
	int m_video_duration_realtime;	// 真实的录像时长 = 录像时长记录定时器 timeout次数
};
