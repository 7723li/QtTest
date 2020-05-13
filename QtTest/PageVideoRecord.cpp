#include "PageVideoRecord.h"

const QString g_test_picname = "./test.jpg";
const QString g_test_videoname = "./test.avi";

PageVideoRecord_kit::PageVideoRecord_kit(QWidget* p)
{
	frame_displayer = new QLabel(p);
	frame_displayer->setGeometry(10, 10, 1186, 885);

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
	connect(m_PageVideoRecord_kit->exit_btn, &QPushButton::clicked, this, &PageVideoRecord::exit_PageVideoRecord);
	connect(m_PageVideoRecord_kit->video_list, &VideoListWidget::video_choosen, this, &PageVideoRecord::slot_replay_begin);
	connect(m_PageVideoRecord_kit->videoplayer, &VideoPlayer_ffmpeg::finish_play_video, this, &PageVideoRecord::slot_replay_finish);

	m_show_frame_timer = new QTimer(this);
	m_show_frame_timer->setTimerType(Qt::TimerType::PreciseTimer);
	connect(m_show_frame_timer, &QTimer::timeout, this, &PageVideoRecord::slot_show_one_frame, Qt::DirectConnection);

	m_record_duration_timer = new QTimer(this);
	m_record_duration_timer->setTimerType(Qt::TimerType::PreciseTimer);
	connect(m_record_duration_timer, &QTimer::timeout, this, &PageVideoRecord::slot_timeout_video_duration_timer);

	m_record_duration_period = 0;

	m_avt_camera = new AVTCamera;
	m_camerabase = m_avt_camera;	// 默认使用AVT摄像头

	m_is_need_capture = false;
	m_is_capturing = false;

	m_is_need_record = false;
	m_is_recording = false;
}

void PageVideoRecord::enter_PageVideoRecord(const QString & examid)
{
	m_examid = examid;

	load_old_videos();						// 加载之前的视频
	clear_videodisplay();					// 清理显示屏
	if (open_camera())						// 打开相机
	{
		int w = 0, h = 0;
		m_camerabase->get_frame_wh(w, h);
		m_mat = cv::Mat(w, h, CV_8UC1);		// 预留帧空间大小

		begin_capture();					// 开始捕捉图像
		begin_show_frame();					// 开始显示画面
	}
	else
	{
		stop_capture();						// 摄像头打开异常 则取消捕捉 并尝试关闭摄像头
		close_camera();
	}
}
void PageVideoRecord::exit_PageVideoRecord()
{
	if (m_is_recording)							// 录像请不要随便乱放 参考陈冠希事件
	{
		PromptBoxInst()->msgbox_go(
			PromptBox_msgtype::Warning,
			PromptBox_btntype::None,
			QStringLiteral("请先停止录像"),
			1000,
			true);
		return;
	}

	stop_show_frame();
	stop_capture();
	close_camera();

	// 发射一个本页面关闭的信号 接不接收无所谓
	emit PageVideoRecord_exit(m_examid);
}

void PageVideoRecord::clear_videodisplay()
{
	// lzx 20200430 将一张deep dark fantasy(纯黑)的图片插入到视频显示窗口 作为初始、失败、错误背景
	QSize dispalyer_size = m_PageVideoRecord_kit->frame_displayer->size();
	QImage image(dispalyer_size, QImage::Format_Grayscale8);
	image.fill(Qt::black);
	m_PageVideoRecord_kit->frame_displayer->setPixmap(QPixmap::fromImage(image));
}

void PageVideoRecord::load_old_videos()
{
	// todo lzx 从数据库中加载
	std::vector<int> index({ 0, 1, 2 });
	for (int i : index)
	{
		put_video_thumb(g_test_videoname, g_test_picname);
	}
}
void PageVideoRecord::put_video_thumb(const QString & video_path, const QString & icon_path)
{
	// 放置缩略图
	QListWidgetItem* new_video_thumbnail = new QListWidgetItem(m_PageVideoRecord_kit->video_list);
	new_video_thumbnail->setIcon(QIcon(QPixmap(icon_path)));
	new_video_thumbnail->setText('T' + QString::number(m_PageVideoRecord_kit->video_list->count()));
	new_video_thumbnail->setTextAlignment(Qt::AlignCenter);
	m_PageVideoRecord_kit->video_list->addItem(new_video_thumbnail);

	// 记录录制的视频名称
	m_recored_videoname_list.push_back(g_test_videoname);
}

bool PageVideoRecord::open_camera()
{
	int openstatus = m_camerabase->openCamera();			// 打开相机
	show_camera_openstatus(openstatus);
	return (camerabase::OpenStatus::OpenSuccess == openstatus);
}
void PageVideoRecord::close_camera()
{
	int close_status = m_camerabase->closeCamera();
	if (camerabase::CloseFailed == close_status)
	{
		PromptBoxInst()->msgbox_go(PromptBox_msgtype::Failed,
			PromptBox_btntype::None,
			QStringLiteral("关闭失败，请尝试拔插\n以重启摄像头"),
			1000,
			true);
	}
}

void PageVideoRecord::show_camera_openstatus(int openstatus)
{
	bool connected = m_camerabase->is_camera_connected();
	if (camerabase::OpenStatus::OpenSuccess == openstatus)
	{
		if (connected)
		{
			PromptBoxInst(m_PageVideoRecord_kit->frame_displayer)->msgbox_go(
				PromptBox_msgtype::Warning, PromptBox_btntype::None, QStringLiteral("相机打开成功"), 2000, true);
		}
		else
		{
			PromptBoxInst(m_PageVideoRecord_kit->frame_displayer)->msgbox_go(
				PromptBox_msgtype::Warning, PromptBox_btntype::None, QStringLiteral("相机打开成功,但数据传输失败"), 2000, true);
		}
	}
	else if (camerabase::OpenStatus::OpenFailed == openstatus)
	{
		PromptBoxInst(m_PageVideoRecord_kit->frame_displayer)->msgbox_go(
			PromptBox_msgtype::Warning, PromptBox_btntype::None,
			QStringLiteral("相机打开失败\n请尝试检查连接并重新插入"), 2000, true);
	}
	else if (camerabase::OpenStatus::NoCameras == openstatus)
	{
		PromptBoxInst(m_PageVideoRecord_kit->frame_displayer)->msgbox_go(
			PromptBox_msgtype::Warning, PromptBox_btntype::None,
			QStringLiteral("没有找到相机\n请尝试检查连接并重新插入"), 2000, true);
	}
}

void PageVideoRecord::begin_capture()
{
	m_is_need_capture = true;
	std::thread capture_thr(std::bind(&PageVideoRecord::capture_thread, this));
	capture_thr.detach();
}
void PageVideoRecord::stop_capture()
{
	m_is_need_capture = false;
	while (m_is_capturing);
}
void PageVideoRecord::capture_thread()
{
	// 从缓冲队列中获取一帧
	while (m_is_need_capture)
	{
		m_is_capturing = true;
		bool is_get_success = m_camerabase->get_one_frame(m_mat);

		if (is_get_success && m_is_need_record)
		{
			m_is_recording = true;
			m_VideoWriter << m_mat;
			m_is_recording = false;
		}
	}

	m_is_capturing = false;
}

bool PageVideoRecord::get_useful_fps(double & fps)
{
	int retry_times = 2;
	fps = m_camerabase->get_framerate();
	while (fps <= 0 && retry_times--)
	{
		::Sleep(1000);
		fps = m_camerabase->get_framerate();
	}

	return (fps > 0);
}

void PageVideoRecord::begin_show_frame()
{	
	double fps = 0;
	if (!get_useful_fps(fps))
	{
		PromptBoxInst()->msgbox_go(
			PromptBox_msgtype::Warning,
			PromptBox_btntype::None,
			QStringLiteral("摄像头似乎出了些问题\n请尝试拔出后重新插入"),
			1000,
			true
			);
		return;
	}
	m_show_frame_timer->start((double)1000 / fps);
}
void PageVideoRecord::stop_show_frame()
{
	m_show_frame_timer->stop();
}

void PageVideoRecord::begin_record()
{
	m_PageVideoRecord_kit->video_time_display->setText("00:00:00");
	m_PageVideoRecord_kit->video_time_display->show();
	m_record_duration_period = 0;

	double fps = 0;
	if (!get_useful_fps(fps))
	{
		PromptBoxInst()->msgbox_go(
			PromptBox_msgtype::Warning,
			PromptBox_btntype::None,
			QStringLiteral("摄像头似乎出了些问题\n请尝试拔出后重新插入"),
			1000,
			true
			);
		return;
	}

	int w = 0, h = 0;
	m_camerabase->get_frame_wh(w, h);

	bool video_writer_opened = m_VideoWriter.open(g_test_videoname.toStdString().c_str(),
		CV_FOURCC('M', 'J', 'P', 'G'), fps, cv::Size(w, h), false);
	if (video_writer_opened)
	{
		m_record_duration_timer->start(1000);
		m_is_need_record = true;
	}
	else
	{
		m_is_need_record = false;
		m_record_duration_timer->stop();
	}
}
void PageVideoRecord::stop_record()
{
	m_is_need_record = false;
	while (m_is_recording);

	m_record_duration_timer->stop();					// 停止计时
	m_VideoWriter.release();							// 保存视频
	m_PageVideoRecord_kit->video_time_display->hide();	// 停止显示时间

	put_video_thumb(g_test_videoname, g_test_picname);	// 放置一个缩略图
}

void PageVideoRecord::slot_show_one_frame()
{
	static const std::map<int, QImage::Format> s_mat_2_image = {
		{ CV_8UC1, QImage::Format_Grayscale8 },
		{ CV_8UC3, QImage::Format_RGB888 },
		{ CV_8UC4, QImage::Format_RGB32 },
	};

	// 显示
	auto img_fmt_iter = s_mat_2_image.find(m_mat.type());
	QImage::Format img_fmt = (img_fmt_iter != s_mat_2_image.end() ? img_fmt_iter->second : QImage::Format_Grayscale8);
	QImage image = QImage(m_mat.data, m_mat.cols, m_mat.rows, img_fmt);
	m_PageVideoRecord_kit->frame_displayer->setPixmap(QPixmap::fromImage(image).
		scaled(m_PageVideoRecord_kit->frame_displayer->size()));
}

void PageVideoRecord::slot_begin_or_finish_record()
{
	if (!m_is_capturing)
	{
		PromptBoxInst(this)->msgbox_go(PromptBox_msgtype::Warning, PromptBox_btntype::None, QStringLiteral("没有摄像头正在运行"), 2000, true);
		return;
	}

	if (!m_record_duration_timer->isActive())
	{
		begin_record();							// 没在录像状态 开始录像
	}
	else
	{
		stop_record();							// 停止录像
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

void PageVideoRecord::slot_replay_begin(QListWidgetItem* choosen_video)
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
		PromptBoxInst()->msgbox_go(PromptBox_msgtype::Warning, 
			PromptBox_btntype::Confirm, 
			QStringLiteral("播放列表似乎出了些问题"), 0, false);
		return;
	}
	
	m_PageVideoRecord_kit->video_list->setEnabled(false);

	const QString & to_play_video_name = m_recored_videoname_list[idx];
	m_PageVideoRecord_kit->videoplayer->play(to_play_video_name);
}
void PageVideoRecord::slot_replay_finish()
{
	m_PageVideoRecord_kit->video_list->setEnabled(true);
}
