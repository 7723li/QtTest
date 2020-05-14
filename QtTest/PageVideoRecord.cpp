#include "PageVideoRecord.h"

const QString g_test_picname = "./test.jpg";
const QString g_test_videoname = "./test.avi";

static const std::map<int, QImage::Format> g_s_mat_2_image = {
	{ CV_8UC1, QImage::Format_Grayscale8 },
	{ CV_8UC3, QImage::Format_RGB888 },
	{ CV_8UC4, QImage::Format_RGB32 },
};

VideoCapture::VideoCapture(PageVideoRecord* p) : 
QThread(p), need_capture(false), need_record(false),
cam(nullptr), mat(nullptr), writer(nullptr)
{

}
bool VideoCapture::begin_capture(camerabase* c, cv::Mat* m, cv::VideoWriter* v)
{
	if (!(c && m && v))
		return false;

	cam = c;
	mat = m;
	writer = v;

	need_capture = true;

	return true;
}
void VideoCapture::stop_capture()
{
	need_capture = false;
	this->wait();
}
void VideoCapture::begin_record()
{
	need_record = true;
}
void VideoCapture::stop_record()
{
	need_record = false;
}
bool VideoCapture::capturing()
{
	return (need_capture || this->isRunning());
}
bool VideoCapture::recording()
{
	if (nullptr == writer)
		return false;
	else
		return (need_record && writer->isOpened() && this->isRunning());
}
void VideoCapture::run()
{
	while (true)
	{		
		if (false == need_capture)
		{
			break;
		}

		bool is_writer_open = writer->isOpened();
		bool is_get_frame_success = cam->get_one_frame(*mat);
		if (is_get_frame_success)
		{
			if (need_record)
			{
				if (is_writer_open)
				{
					writer->write(*mat);
				}
			}
			else
			{
				if (is_writer_open)
				{
					writer->release();
				}

			}
		}
	}
}

TransformPicture::TransformPicture(PageVideoRecord* p):
QThread(p), _mat(nullptr), _run(false)
{

}
bool TransformPicture::begin_transform(cv::Mat* mat, QSize show_size, double sleep_time)
{
	if (nullptr == mat || show_size.width() == 0 || show_size.height() == 0)
	{
		return false;
	}

	_mat = mat;
	_show_size = show_size;
	_sleep_time = sleep_time;
	_run = true;

	return true;
}
void TransformPicture::stop_transform()
{
	_run = false;
	this->wait();
}
void TransformPicture::run()
{
	while (true)
	{
		if (false == _run)
		{
			break;
		}

		auto img_fmt_iter = g_s_mat_2_image.find(_mat->type());
		QImage::Format img_fmt = (img_fmt_iter != g_s_mat_2_image.end() ? img_fmt_iter->second : QImage::Format_Grayscale8);
		const QImage& image = QImage(_mat->data, _mat->cols, _mat->rows, img_fmt);

		emit show_one_frame(QPixmap::fromImage(image).scaled(_show_size));
		::Sleep(_sleep_time);
	}
}

PageVideoRecord_kit::PageVideoRecord_kit(PageVideoRecord* p)
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

	m_tranpicthr = new TransformPicture(this);
	connect(m_tranpicthr, &TransformPicture::show_one_frame, this, &PageVideoRecord::slot_show_one_frame);

	m_record_duration_timer = new QTimer(this);
	m_record_duration_timer->setTimerType(Qt::TimerType::PreciseTimer);
	connect(m_record_duration_timer, &QTimer::timeout, this, &PageVideoRecord::slot_show_recordtime);

	m_record_duration_period = QTime(0, 0, 0, 0);

	m_video_capture = new VideoCapture(this);

	m_avt_camera = new AVTCamera;
	m_camerabase = m_avt_camera;	// 默认使用AVT摄像头
}

void PageVideoRecord::enter_PageVideoRecord(const QString & examid)
{
	m_examid = examid;

	load_old_vidthumb();						// 加载之前的视频缩略图
	clear_videodisplay();						// 清理显示屏
	int sta = -1;
	sta = m_camerabase->openCamera();			// 打开相机
	show_camera_openstatus(sta);				// 提示相机打开状态
	if (sta == camerabase::OpenSuccess)			// 打开成功
	{
		int w = 0, h = 0;
		m_camerabase->get_frame_wh(w, h);
		m_mat = cv::Mat::zeros(w, h, CV_8UC1);	// 分配帧空间大小

		begin_capture();						// 开始捕捉图像
		begin_show_frame();						// 开始显示图像
	}
	else										// 摄像头打开异常
	{
		stop_show_frame();						// 取消显示
		stop_capture();							// 取消捕捉
		sta = m_camerabase->closeCamera();		// 关闭摄像头
		show_camera_closestatus(sta);			// 提示关闭状态
	}
}
void PageVideoRecord::exit_PageVideoRecord()
{
	if (m_video_capture->recording())			// 录像请不要随便处置 参考陈冠希老师
	{
		PromptBoxInst()->msgbox_go(
			PromptBox_msgtype::Warning,
			PromptBox_btntype::None,
			QStringLiteral("请先停止录制"),
			1000,
			true);
		return;
	}

	stop_show_frame();						// 停止显示
	stop_capture();							// 停止捕捉影像
	m_camerabase->closeCamera();			// 关闭摄像头
	clear_all_vidthumb();					// 清理缩略图

	emit PageVideoRecord_exit(m_examid);	// 发射一个本页面关闭的信号
}

void PageVideoRecord::clear_videodisplay()
{
	// lzx 20200430 将一张deep dark fantasy(纯黑)的图片插入到视频显示窗口 作为初始、失败、错误的显示背景
	QSize dispalyer_size = m_PageVideoRecord_kit->frame_displayer->size();
	static QImage image(dispalyer_size, QImage::Format_Grayscale8);
	if (image.size() != dispalyer_size)
	{
		image = image.scaled(dispalyer_size);
	}
	image.fill(Qt::black);
	m_PageVideoRecord_kit->frame_displayer->setPixmap(QPixmap::fromImage(image));
}

void PageVideoRecord::load_old_vidthumb()
{
	// todo lzx 从数据库中加载
	//DB::LOAD_FROM_EXAMID(m_examid);

	int thumb_num = m_PageVideoRecord_kit->video_list->count();
	if (thumb_num > 0)
		return;

	std::vector<int> index({ 0, 1, 2 });
	for (int i : index)
	{
		put_video_vidthumb(g_test_videoname, g_test_picname);
	}
}
void PageVideoRecord::put_video_vidthumb(const QString & video_path, const QString & icon_path)
{
	// 放置缩略图
	QListWidgetItem* new_video_thumbnail = new QListWidgetItem(m_PageVideoRecord_kit->video_list);
	new_video_thumbnail->setIcon(QIcon(QPixmap(icon_path)));
	new_video_thumbnail->setText('T' + QString::number(m_PageVideoRecord_kit->video_list->count()));
	new_video_thumbnail->setTextAlignment(Qt::AlignCenter);
	m_PageVideoRecord_kit->video_list->addItem(new_video_thumbnail);

	// 记录录制的视频名称
	m_map_video_thumb_2_name[new_video_thumbnail] = video_path;
}
void PageVideoRecord::clear_all_vidthumb()
{
	m_map_video_thumb_2_name.clear();
	int thumb_num = m_PageVideoRecord_kit->video_list->count();
	for (int i = 0; i < thumb_num; ++i)
	{
		QListWidgetItem* thumb = m_PageVideoRecord_kit->video_list->item(i);
		if (nullptr != thumb)
		{
			delete thumb;
			thumb = nullptr;
		}
	}
	m_PageVideoRecord_kit->video_list->clear();
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
void PageVideoRecord::show_camera_closestatus(int closestatus)
{
	if (camerabase::CloseFailed == closestatus)
	{
		PromptBoxInst()->msgbox_go(PromptBox_msgtype::Failed,
			PromptBox_btntype::None,
			QStringLiteral("关闭失败，请尝试拔插\n以重启摄像头"),
			1000,
			true);
	}
}

void PageVideoRecord::begin_capture()
{
	bool succ = m_video_capture->begin_capture(m_camerabase, &m_mat, &m_VideoWriter);
	if (succ)
	{
		m_video_capture->start();
	}
}
void PageVideoRecord::stop_capture()
{
	m_video_capture->stop_capture();
	while (m_video_capture->capturing());
}

bool PageVideoRecord::get_useful_fps(double & fps)
{
	// 获取的帧率<=0 有可能是因为相机挂掉了或者相机刚刚开启 等两秒再看看
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
	QSize show_size = m_PageVideoRecord_kit->frame_displayer->size();
	double sleep_time = (double)1000 / fps;
	bool succ = m_tranpicthr->begin_transform(&m_mat, show_size, sleep_time);
	if (succ)
	{
		m_tranpicthr->start();
	}
}
void PageVideoRecord::stop_show_frame()
{
	m_tranpicthr->stop_transform();
}
void PageVideoRecord::slot_show_one_frame(const QPixmap& _pixmap)
{
	m_PageVideoRecord_kit->frame_displayer->setPixmap(_pixmap);
}

void PageVideoRecord::begin_record()
{
	// 更改视频捕捉线程中的录制状态为TRUE 从下一轮捕捉开始录制
	m_video_capture->begin_record();
}
void PageVideoRecord::stop_record()
{
	// 更改视频捕捉线程中的录制状态为FALSE 从下一轮捕捉停止录制 并释放视频头
	m_video_capture->stop_record();
	while (m_video_capture->recording());
}
void PageVideoRecord::slot_begin_or_finish_record()
{
	bool captur_ing = m_video_capture->capturing();
	if (!captur_ing)
	{
		PromptBoxInst(this)->msgbox_go(PromptBox_msgtype::Warning, PromptBox_btntype::None, QStringLiteral("没有摄像头正在运行"), 2000, true);
		return;
	}

	bool record_ing = m_video_capture->recording();
	if (!record_ing)
	{
		bool init_header = apply_record_msg();					// 创建视频头 录制关键 没有必挂
		if (!init_header)
		{
			return;
		}
		m_PageVideoRecord_kit->video_list->setEnabled(false);	// 录制期间不可回播
		begin_show_recordtime();								// 开始显示录制时间
		begin_record();											// 没在录像状态 开始录像
	}
	else
	{
		stop_record();											// 停止录像 并 在下一个循环释放视频头
		stop_show_recordtime();									// 隐藏录制时间
		put_video_vidthumb(g_test_videoname, g_test_picname);	// 放置一个缩略图
		m_PageVideoRecord_kit->video_list->setEnabled(true);	// 录制完毕 可以回播
	}
}

bool PageVideoRecord::apply_record_msg()
{
	double fps = 0;											// 录制帧率
	if (!get_useful_fps(fps))
	{
		PromptBoxInst()->msgbox_go(
			PromptBox_msgtype::Warning,
			PromptBox_btntype::None,
			QStringLiteral("摄像头似乎出了些问题\n请尝试拔出后重新插入"),
			1000,
			true
			);
		return false;
	}

	int w = 0, h = 0;										// 视频长宽
	m_camerabase->get_frame_wh(w, h);

	m_VideoWriter.open(										// 创建视频头
		g_test_videoname.toStdString().c_str(),
		CV_FOURCC('M', 'J', 'P', 'G'),
		fps, cv::Size(w, h), false
		);

	return m_VideoWriter.isOpened();
}

void PageVideoRecord::begin_show_recordtime()
{
	m_record_duration_period = QTime(0, 0, 0, 0);			// 录制显示的时长时分秒初始化常为全0
	m_PageVideoRecord_kit->video_time_display->setText(m_record_duration_period.toString("hh:mm:ss"));
	m_PageVideoRecord_kit->video_time_display->show();
	m_record_duration_timer->start(1000);					// 开始记录录制时间
}
void PageVideoRecord::stop_show_recordtime()
{
	m_record_duration_timer->stop();						// 停止计时
	m_PageVideoRecord_kit->video_time_display->hide();		// 停止显示时间
}
void PageVideoRecord::slot_show_recordtime()
{
	m_record_duration_period = m_record_duration_period.addSecs(1);
	m_PageVideoRecord_kit->video_time_display->setText(m_record_duration_period.toString("hh:mm:ss"));

	// 最长只可以录一个钟
	if (m_record_duration_period.hour() >= 1)
	{
		stop_record();
	}
}

void PageVideoRecord::slot_replay_begin(QListWidgetItem* choosen_video)
{
	if (nullptr == choosen_video)
		return;

	const thumb_name::iterator& video_name_iter = m_map_video_thumb_2_name.find(choosen_video);
	if (video_name_iter == m_map_video_thumb_2_name.end())
	{
		PromptBoxInst()->msgbox_go(PromptBox_msgtype::Warning,
			PromptBox_btntype::Confirm,
			QStringLiteral("播放列表似乎出了些问题"), 0, false);
		return;
	}

	stop_show_frame();
	m_PageVideoRecord_kit->video_list->setEnabled(false);				// 回播期间 避免出bug 不允许点击其他视频
	m_PageVideoRecord_kit->videoplayer->play(video_name_iter->second);	// 开始播放视频
}
void PageVideoRecord::slot_replay_finish()
{
	begin_show_frame();
	m_PageVideoRecord_kit->video_list->setEnabled(true);				// 回播完 我房间里有些其他好康的
}
