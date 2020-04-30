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

#include <ctime>

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QHeaderView>
#include <QTime>
#include <QTimer>

#include "PromptBox.h"
#include "VideoListWidget.h"
#include "externalFile/opencv/include/opencv2/opencv.hpp"

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
	QLabel* video_time_display;			// 视频时长显示
	QPushButton* record_btn;			// (停止)录制按钮
	QPushButton* exit_btn;				// 结束检查按钮

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

public:
	/*
	@brief
	外部切换界面入口
	@param[1] examid 病例id QString
	*/
	void prepare_record(const QString & examid);

private:
	void clear_video_displayer();

private slots:
	void slot_timeout_getframe();				// 以1ms的频率 去相机处取1帧
	void slot_begin_or_finish_record();			// 开始(结束)录制视频
	void slot_timeout_video_duration_timer();	// 相机录制计时
	void slot_exit();							// 退出界面

signals:
	void PageVideoRecord_exit(const QString & examid);

private:
	PageVideoRecord_kit* m_PageVideoRecord_kit;

	QString m_examid;

	QTimer* m_video_getframe_timer;		// 用于取帧的定时器

	QTimer* m_record_duration_timer;	// 用于记录录像时长的定时器
	int m_record_duration_period;		// 录像时长定时器溢出次数 = 录像时长秒数

	cv::Mat m_camera_capture_mat;
	cv::VideoWriter m_VideoWriter;

	AVTCamera* avt_camera;
};
