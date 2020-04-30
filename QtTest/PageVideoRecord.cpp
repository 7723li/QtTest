#include "PageVideoRecord.h"

PageVideoRecord_kit::PageVideoRecord_kit(QWidget* p)
{
	video_displayer = new QLabel(p);
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

	for (int i = 0; i < 10; ++i)
	{
		QListWidgetItem* iitem = new QListWidgetItem(video_list);
		iitem->setIcon(QIcon(QPixmap("E:/codes/TopV/V100/V2.85+/TopV2/data/image/20191231170125cut_0_60.bmp")));
		iitem->setText('T' + QString::number(i));
		iitem->setTextAlignment(Qt::AlignCenter);

		video_list->addItem(iitem);
	}
}

PageVideoRecord::PageVideoRecord(QWidget* parent) :
	QWidget(parent)
{
	this->setStyleSheet("background-color: rgb(251,251,251);");
	//this->setObjectName("widgetBottomShelter");

	m_PageVideoRecord_kit = new PageVideoRecord_kit(this);
	connect(m_PageVideoRecord_kit->record_btn, &QPushButton::clicked, this, &PageVideoRecord::slot_begin_or_finish_record);
	connect(m_PageVideoRecord_kit->exit_btn, &QPushButton::clicked, this, &PageVideoRecord::slot_exit);

	m_video_getframe_timer = new QTimer(this);
	m_video_getframe_timer->setTimerType(Qt::TimerType::PreciseTimer);
	connect(m_video_getframe_timer, &QTimer::timeout, this, &PageVideoRecord::slot_timeout_getframe);

	m_record_duration_timer = new QTimer(this);
	m_record_duration_timer->setTimerType(Qt::TimerType::PreciseTimer);
	connect(m_record_duration_timer, &QTimer::timeout, this, &PageVideoRecord::slot_timeout_video_duration_timer);
	
	m_record_duration_period = 0;
}

void PageVideoRecord::prepare_record(const QString & examid)
{
	m_examid = examid;

	clear_video_displayer();

	//Camera* cam = Camera::OpenCamera();
	//if (nullptr != cam && 0 == cam->openCamera())
	//{
	//	m_video_getframe_timer->start(1); // 开始取帧
	//}
}

void PageVideoRecord::clear_video_displayer()
{
	// lzx 20200430 将一张deep dark fantasy的图片插入到视频显示窗口 作为初始、失败、错误背景
	static QPixmap pixmap;
	pixmap.scaled(m_PageVideoRecord_kit->video_displayer->size());
	pixmap.fill(Qt::black);
	m_PageVideoRecord_kit->video_displayer->setPixmap(pixmap);
}

void PageVideoRecord::slot_timeout_getframe()
{
	//Camera::CopyCurrentFrame(&m_camera_capture_mat);

	// 图像显示
	const QPixmap* to_show_pixmap = m_PageVideoRecord_kit->video_displayer->pixmap();
	static QImage to_show_iamge;
	if (!m_camera_capture_mat.empty())
	{
		to_show_iamge.scaled(m_camera_capture_mat.cols, m_camera_capture_mat.rows);
		to_show_iamge.loadFromData(m_camera_capture_mat.data, QImage::Format_Indexed8);

		to_show_pixmap->fromImage(to_show_iamge);
	}

	// 保存视频
	if (m_record_duration_timer->isActive())
	{
		m_VideoWriter.write(m_camera_capture_mat);
	}
}

void PageVideoRecord::slot_begin_or_finish_record()
{
	if (!m_record_duration_timer->isActive())	// 没在录像状态 开始录像
	{
		m_PageVideoRecord_kit->video_time_display->show();
		m_record_duration_period = 0;
		m_PageVideoRecord_kit->video_time_display->setText("00:00:00");
		m_record_duration_timer->start(1000);		
	}
	else										// 停止录像
	{
		m_PageVideoRecord_kit->video_time_display->hide();
		m_record_duration_timer->stop();
		m_video_getframe_timer->stop();
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

void PageVideoRecord::slot_exit()
{
	if (m_record_duration_timer->isActive())	// 录像中 参考陈冠希事件
	{
		PromptBoxInst->msgbox_go(
			PromptBox_msgtype::Warning,
			PromptBox_btntype::None,
			QStringLiteral("请先停止录像"),
			1000,
			true);
		return;
	}

	//Camera::CloseCamera();
	emit PageVideoRecord_exit(m_examid);
}
