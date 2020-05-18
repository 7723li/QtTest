#include "PageVideoRecord.h"

const QString g_test_picname = "./test.jpg";
const QString g_test_videoname = "./test.avi";
static QMutex g_mutex;

CameraCapture::CameraCapture(PageVideoRecord* p) : 
QThread(p), m_need_capture(false), m_need_record(false),
m_cam(nullptr), m_mat(nullptr), m_writer(nullptr)
{

}
bool CameraCapture::begin_capture(camerabase* c, cv::Mat* m, cv::VideoWriter* v)
{
	if (!(c && m && v))
		return false;

	m_cam = c;
	m_mat = m;
	m_writer = v;

	m_need_capture = true;

	return true;
}
void CameraCapture::stop_capture()
{
	m_need_capture = false;
	this->wait();
}
void CameraCapture::begin_record()
{
	m_need_record = true;
}
void CameraCapture::stop_record()
{
	m_need_record = false;
}
bool CameraCapture::capturing()
{
	return (m_need_capture || this->isRunning());
}
bool CameraCapture::recording()
{
	if (nullptr == m_writer)
		return false;
	else
		return (m_need_record && m_writer->isOpened() && this->isRunning());
}
void CameraCapture::run()
{
	while (true)
	{		
		if (false == m_need_capture)
		{
			break;
		}

		g_mutex.lock();
		bool is_get_frame_success = m_cam->get_one_frame(*m_mat);
		g_mutex.unlock();

		if (is_get_frame_success)
		{
			if (m_need_record)
			{
				m_writer->write(*m_mat);
			}
			else
			{
				m_writer->release();
			}
		}
		else
		{
			::Sleep(3);
		}
	}
}

TransformPicture::TransformPicture(PageVideoRecord* p):
QThread(p), m_run(false)
{

}
bool TransformPicture::begin_transform(cv::Mat* mat, QSize qs, double st)
{
	if (nullptr == mat || mat->empty() || qs.width() <= 0 || qs.height() <= 0)
	{
		return false;
	}

	m_mat = mat;
	m_show_size = qs;
	m_sleep_time = st;
	m_run = true;

	switch (m_mat->type())
	{
	case CV_8UC1:
		m_fmt = QImage::Format_Grayscale8;
		break;
	case CV_8UC3:
		m_fmt = QImage::Format_RGB888;
		break;
	case CV_8UC4:
		m_fmt = QImage::Format_RGB32;
		break;
	default:
		m_fmt = QImage::Format_Grayscale8;
		break;
	}

	return true;
}
void TransformPicture::stop_transform()
{
	m_run = false;
	this->wait();
}
void TransformPicture::run()
{
	while (true)
	{
		if (false == m_run)
		{
			break;
		}

		g_mutex.lock();
		QImage& image = QImage(m_mat->data, m_mat->cols, m_mat->rows, m_fmt);
		g_mutex.unlock();

		QPixmap& pix = QPixmap::fromImage(image);
		pix = pix.scaled(m_show_size);
		emit show_one_frame(pix);

		::Sleep(m_sleep_time);
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
	QWidget(parent),
	m_ready_record_hint(QStringLiteral("准备录像中.."))
{
	this->setStyleSheet("background-color: rgb(251,251,251);");
	this->setFocusPolicy(Qt::ClickFocus);
	//this->setObjectName("widgetBottomShelter");

	m_PageVideoRecord_kit = new PageVideoRecord_kit(this);
	connect(m_PageVideoRecord_kit->record_btn, &QPushButton::clicked, this, &PageVideoRecord::slot_begin_or_finish_record);
	connect(m_PageVideoRecord_kit->exit_btn, &QPushButton::clicked, this, &PageVideoRecord::exit_PageVideoRecord);
	connect(m_PageVideoRecord_kit->video_list, &VideoListWidget::video_choosen, this, &PageVideoRecord::slot_replay_begin);
	connect(m_PageVideoRecord_kit->videoplayer, &VideoPlayer_ffmpeg::play_finish, this, &PageVideoRecord::slot_replay_finish);

	m_tranpicthr = new TransformPicture(this);
	connect(m_tranpicthr, &TransformPicture::show_one_frame, this, &PageVideoRecord::slot_show_one_frame, Qt::DirectConnection);

	m_record_duration_timer = new QTimer(this);
	m_record_duration_timer->setTimerType(Qt::TimerType::PreciseTimer);
	connect(m_record_duration_timer, &QTimer::timeout, this, &PageVideoRecord::slot_show_recordtime);

	m_record_duration_period = QTime(0, 0, 0, 0);

	m_show_framerate_timer = new QTimer(this);
	m_show_framerate_timer->setTimerType(Qt::TimerType::PreciseTimer);
	connect(m_show_framerate_timer, &QTimer::timeout, this, &PageVideoRecord::slot_show_framerate);

	m_is_show_framerate = false;
	m_framerate = 0.0;

	m_video_capture = new CameraCapture(this);

	m_avt_camera = new AVTCamera;
	m_camerabase = m_avt_camera;	// 默认使用AVT摄像头
}

void PageVideoRecord::enter_PageVideoRecord(const QString & examid)
{
	m_examid = examid;
	m_video_path = g_test_videoname;
	m_video_thumb_path = g_test_picname;
	m_is_show_framerate = false;
	m_framerate = 0;

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

		begin_capture();						// 开启捕捉线程 开始捕捉图像
		begin_collect_fps();					// 开始收集fps
		begin_show_frame();						// 开启转换线程 开始显示图像
	}
	else										// 摄像头打开异常
	{
		stop_capture();							// 关闭捕捉线程 取消捕捉
		stop_show_frame();						// 关闭图像转换线程 取消显示
		sta = m_camerabase->closeCamera();		// 关闭摄像头
		show_camera_closestatus(sta);			// 提示关闭状态
		stop_collect_fps();						// 停止采集fps
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

	stop_collect_fps();							// 停止采集fps
	stop_capture();								// 关闭捕捉线程 取消捕捉
	stop_show_frame();							// 关闭图像转换线程 取消显示
	m_camerabase->closeCamera();				// 关闭摄像头
	clear_all_vidthumb();						// 清理缩略图

	emit PageVideoRecord_exit(m_examid);		// 发射一个本页面关闭的信号
}

void PageVideoRecord::keyPressEvent(QKeyEvent* e)
{
	if (e->modifiers() == Qt::ControlModifier && Qt::Key_V == e->key())
	{
		e->accept();
		m_is_show_framerate = true;
	}
	else if (e->modifiers() == Qt::ControlModifier && Qt::Key_B == e->key())
	{
		m_is_show_framerate = false;
		e->accept();
	}
	else
	{
		e->ignore();
	}
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
		const QString& old_video_name = g_test_videoname;
		const QString& old_video_thm_name = g_test_videoname;
		put_video_vidthumb(old_video_name, old_video_thm_name);
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

void PageVideoRecord::begin_collect_fps()
{
	m_show_framerate_timer->start(1000);
}
void PageVideoRecord::stop_collect_fps()
{
	m_show_framerate_timer->stop();
}

void PageVideoRecord::begin_show_frame()
{
	QSize show_size = m_PageVideoRecord_kit->frame_displayer->size();
	double sleep_time = (double)1000 / 30;
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
void PageVideoRecord::slot_show_one_frame(QPixmap& _pixmap)
{
	if (m_is_show_framerate)
	{
		QPainter painter(&_pixmap);
		painter.setPen(Qt::blue);
		painter.drawText(QRect(0, 0, 200, 40),QString("fps : ") + QString::number(m_framerate));
	}

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
		put_video_vidthumb(m_video_path, m_video_thumb_path);	// 放置一个缩略图
		m_PageVideoRecord_kit->video_list->setEnabled(true);	// 录制完毕 可以回播
	}
}

bool PageVideoRecord::apply_record_msg()
{
	while (m_framerate <= 0);
	int w = 0, h = 0;									// 视频长宽
	m_camerabase->get_frame_wh(w, h);

	if (m_VideoWriter.isOpened())
	{
		m_VideoWriter.release();
	}

	bool open_succ = m_VideoWriter.open(				// 创建视频头
		m_video_path.toStdString().c_str(),
		CV_FOURCC('M', 'J', 'P', 'G'),
		m_framerate, cv::Size(w, h), false
		);

	return open_succ;
}

void PageVideoRecord::begin_show_recordtime()
{
	m_record_duration_period = QTime(0, 0, 0, 0);				// 录制显示的时长时分秒初始化常为全0
	m_PageVideoRecord_kit->video_time_display->setText(m_ready_record_hint);
	m_PageVideoRecord_kit->video_time_display->show();
	m_record_duration_timer->start(1000);						// 开始记录录制时间
}
void PageVideoRecord::stop_show_recordtime()
{
	m_record_duration_timer->stop();							// 停止计时
	m_PageVideoRecord_kit->video_time_display->hide();			// 停止显示时间
}
void PageVideoRecord::slot_show_recordtime()
{
	// 最长只可以录一个钟
	if (m_record_duration_period.hour() >= 1)
	{
		return stop_record();
	}

	m_PageVideoRecord_kit->video_time_display->setText(m_record_duration_period.toString("hh:mm:ss"));
	m_record_duration_period = m_record_duration_period.addSecs(1);
}

void PageVideoRecord::slot_show_framerate()
{
	m_framerate = m_camerabase->get_framerate();
	qDebug() << m_framerate;
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

	m_PageVideoRecord_kit->video_list->setEnabled(false);				// 回播期间 避免出bug 不允许点击其他视频
	m_PageVideoRecord_kit->videoplayer->show();
	m_PageVideoRecord_kit->videoplayer->play(video_name_iter->second);	// 开始播放视频
}
void PageVideoRecord::slot_replay_finish()
{
	m_PageVideoRecord_kit->video_list->setEnabled(true);				// 回播完 我房间里有些其他好康的
}
