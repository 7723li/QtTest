#include "PageVideoRecord.h"

const QString g_test_picname = "./test.jpg";
const QString g_test_videoname = "./test.avi";

PageVideoRecord_kit::PageVideoRecord_kit(QWidget* p)
{
	video_displayer = new VideoDisplayWidget(p);
	video_displayer->setGeometry(10, 10, 1186, 885);

	bed_num_label = new QLabel(p);
	bed_num_label->setText(QStringLiteral("你是傻逼"));
	bed_num_label->setAlignment(Qt::AlignCenter);
	bed_num_label->setGeometry(1250, 10, 100, 40);
	bed_num_label->setStyleSheet("background-color: rgb(251,251,251);border:2px solid #242424;border-radius:5px;");

	patient_name_label = new QLabel(p);
	patient_name_label->setText(QStringLiteral("你是傻屌"));
	patient_name_label->setAlignment(Qt::AlignCenter);
	patient_name_label->setGeometry(1400, 10, 100, 40);
	patient_name_label->setStyleSheet("background-color: rgb(251,251,251);border:2px solid #242424;border-radius:5px;");

	video_time_display = new QLabel(p);
	video_time_display->setGeometry(1400, 300, 100, 30);
	video_time_display->setStyleSheet("background-color: rgb(251,251,251);");
	video_time_display->hide();	// 初始默认不显示

	record_btn = new QPushButton(p);
	//record_btn->setObjectName("pushButtonVideo");
	record_btn->setStyleSheet("background-color: rgb(0,187,163);color: #ffffff;");
	record_btn->setIcon(QIcon("./Resources/icon/PauseVideo2.png"));
	record_btn->setText(QStringLiteral("录像"));
	record_btn->setGeometry(1500, 300, 200, 80);

	exit_btn = new QPushButton(p);
	//exit_btn->setObjectName("pushButtonVideo");
	exit_btn->setStyleSheet("background-color: rgb(0,187,163);color: #ffffff;");
	exit_btn->setIcon(QIcon("./Resources/icon/PauseVideo2.png"));
	exit_btn->setText(QStringLiteral("结束本床检查"));
	exit_btn->setGeometry(1600, 500, 150, 30);

	video_list_background = new QWidget(p);
	video_list_background->setGeometry(1196, 600, 1920-1200, 400);

	video_list = new VideoListWidget(video_list_background);
	video_list->setGeometry(0, 20, video_list_background->width(), 120);
	video_list->setIconSize(QSize(100, 100));
	video_list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	video_list->setModelColumn(10);
	video_list->setFlow(QListView::LeftToRight);
	video_list->setViewMode(QListView::IconMode);
	video_list->setWrapping(false);
	video_list->setSpacing(20);

	videoplayer = new VideoPlayer_ffmpeg(p);
	videoplayer->hide();								// 默认隐藏
	videoplayer->setGeometry(100, 100, 1000, 1000);
}

PageVideoRecord::PageVideoRecord(QWidget* parent) :
	QWidget(parent)
{
	this->setStyleSheet("background-color: rgb(251,251,251);");
	//this->setObjectName("widgetBottomShelter");

	m_PageVideoRecord_kit = new PageVideoRecord_kit(this);
	connect(m_PageVideoRecord_kit->record_btn, &QPushButton::clicked, this, &PageVideoRecord::slot_begin_or_finish_record);
	connect(m_PageVideoRecord_kit->exit_btn, &QPushButton::clicked, this, &PageVideoRecord::slot_exit);
	connect(m_PageVideoRecord_kit->video_list, &VideoListWidget::video_choosen, this, &PageVideoRecord::slot_replay_recordedvideo);

	m_get_frame_timer = new QTimer(this);
	m_get_frame_timer->setTimerType(Qt::TimerType::PreciseTimer);
	connect(m_get_frame_timer, &QTimer::timeout, this, &PageVideoRecord::slot_get_one_frame);

	m_record_duration_timer = new QTimer(this);
	m_record_duration_timer->setTimerType(Qt::TimerType::PreciseTimer);
	connect(m_record_duration_timer, &QTimer::timeout, this, &PageVideoRecord::slot_timeout_video_duration_timer);

	m_record_duration_period = 0;

	m_avt_camera = new AVTCamera;
	m_camerabase = m_avt_camera;	// 默认使用AVT摄像头
}

void PageVideoRecord::prepare_record(const QString & examid)
{
	m_examid = examid;

	clear_video_displayer();

	/* @todo 从数据库中获取之前录制过的视频 */
	// m_recored_video_list = TopVDB::getRecoredVideo(examid);

	int w = 0, h = 0;
	int closeCam_res = m_camerabase->closeCamera();		// 打开前先关闭 重置相机 以免之前有错误操作导致拉胯
	int openCamera_res = m_camerabase->openCamera();	// 打开相机
	switch (openCamera_res)
	{
	case camerabase::OpenStatus::OpenSuccess:
		m_camerabase->get_frame_wh(w, h);				// 重置用来接收数据的帧
		if (!m_mat.empty())
			m_mat.release();
		m_mat = cv::Mat(h, w, CV_8UC1);
		m_get_frame_timer->start(1);					// 开启定时器 开始获取帧并显示 理论频率1s/1ms=1000Hz 开足马力

		// 提示成功并借机等待2秒 等待过程中相机运行稳定然后可以获取比较准确的fps数值 可用于写入视频
		PromptBoxInst(m_PageVideoRecord_kit->video_displayer)->msgbox_go(PromptBox_msgtype::Warning, PromptBox_btntype::None, QStringLiteral("相机打开成功"), 2000, true);
		break;
	case camerabase::OpenStatus::OpenFailed:
		PromptBoxInst(m_PageVideoRecord_kit->video_displayer)->msgbox_go(PromptBox_msgtype::Warning, PromptBox_btntype::None, QStringLiteral("相机打开失败，请尝试检查连接并重新插入"), 2000, true);
		break;
	case camerabase::OpenStatus::NoCameras:
		PromptBoxInst(m_PageVideoRecord_kit->video_displayer)->msgbox_go(PromptBox_msgtype::Warning, PromptBox_btntype::None, QStringLiteral("没有找到相机，请尝试检查连接并重新插入"), 2000, true);
		break;
	default:
		break;
	}
}

void PageVideoRecord::clear_video_displayer()
{
	// lzx 20200430 将一张deep dark fantasy的图片插入到视频显示窗口 作为初始、失败、错误背景
	if (m_mat.empty())
	{
		m_mat = cv::Mat(m_PageVideoRecord_kit->video_displayer->height(), m_PageVideoRecord_kit->video_displayer->width(), CV_8UC1);
	}
	memset(m_mat.data, 1, m_mat.cols * m_mat.rows);
	m_PageVideoRecord_kit->video_displayer->show_frame(&m_mat);
}

void PageVideoRecord::slot_get_one_frame()
{
	if (false == m_camerabase->get_one_frame(&m_mat))
		return;

	// 显示
	if (!m_mat.empty())
	{
		m_PageVideoRecord_kit->video_displayer->show_frame(&m_mat);
		m_PageVideoRecord_kit->video_displayer->update();
	}

	// 写入帧
	if (m_record_duration_timer->isActive())
	{
		if (m_VideoWriter.isOpened())
		{
			m_VideoWriter.write(m_mat);
		}
	}
}

void PageVideoRecord::slot_begin_or_finish_record()
{
	if (!m_get_frame_timer->isActive())
	{
		PromptBoxInst()->msgbox_go(PromptBox_msgtype::Warning, PromptBox_btntype::None, QStringLiteral("没有摄像头正在运行"), 2000, true);
		return;
	}

	if (!m_record_duration_timer->isActive())	// 没在录像状态 开始录像
	{
		m_PageVideoRecord_kit->video_time_display->show();
		m_record_duration_period = 0;
		m_PageVideoRecord_kit->video_time_display->setText("00:00:00");

		if(m_VideoWriter.open(g_test_videoname.toStdString().c_str(), CV_FOURCC('M', 'J', 'P', 'G'), 120, cv::Size(m_mat.cols, m_mat.rows), false))
			m_record_duration_timer->start(1000);
	}
	else										// 停止录像
	{
		m_PageVideoRecord_kit->video_time_display->hide();
		m_record_duration_timer->stop();

		// 放置缩略图
		QListWidgetItem* new_video_thumbnail = new QListWidgetItem(m_PageVideoRecord_kit->video_list);
		new_video_thumbnail->setIcon(QIcon(QPixmap(g_test_picname)));
		new_video_thumbnail->setText('T' + QString::number(m_PageVideoRecord_kit->video_list->count()));
		new_video_thumbnail->setTextAlignment(Qt::AlignCenter);
		m_PageVideoRecord_kit->video_list->addItem(new_video_thumbnail);

		// 放入录制的视频名称
		m_recored_videoname_list.push_back(g_test_videoname);

		// 保存视频
		m_VideoWriter.release();
	}
}

void PageVideoRecord::slot_timeout_video_duration_timer()
{
	++m_record_duration_period;
	QTime video_show_time(0, 0, 0, 0);
	video_show_time = video_show_time.addSecs(m_record_duration_period);
	m_PageVideoRecord_kit->video_time_display->setText(video_show_time.toString("hh:mm:ss"));

	// 最长只可以录一个钟
	if (video_show_time.hour() >= 1)
	{
		slot_begin_or_finish_record();
	}
}

void PageVideoRecord::slot_replay_recordedvideo(QListWidgetItem* choosen_video)
{
	if (nullptr == choosen_video)
		return;

	int idx = 0;
	for (idx = 0; idx < m_PageVideoRecord_kit->video_list->count(); ++idx)
	{
		QListWidgetItem* video_item = m_PageVideoRecord_kit->video_list->item(idx);
		if (video_item == choosen_video)
			break;
	}

	if (idx >= m_recored_videoname_list.size())
	{
		PromptBoxInst()->msgbox_go(PromptBox_msgtype::Warning, PromptBox_btntype::Confirm, QStringLiteral("播放列表似乎出了些问题"), 0, false);
		return;
	}
	
	const QString & to_play_video_name = m_recored_videoname_list[idx];
	// @todo 播放

	m_PageVideoRecord_kit->videoplayer->play(to_play_video_name);
}

void PageVideoRecord::slot_exit()
{
	if (m_record_duration_timer->isActive())	// 录像请不要随便乱放 参考陈冠希事件
	{
		PromptBoxInst()->msgbox_go(
			PromptBox_msgtype::Warning,
			PromptBox_btntype::None,
			QStringLiteral("请先停止录像"),
			1000,
			true);
		return;
	}

	do
	{
		m_get_frame_timer->stop();
	}
	while (m_get_frame_timer->isActive());
	int close_status = m_camerabase->closeCamera();

	// 发射一个本页面关闭的信号 接不接收无所谓
	emit PageVideoRecord_exit(m_examid);
}
