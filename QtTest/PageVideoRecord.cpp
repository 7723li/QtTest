#include "PageVideoRecord.h"

PageVideoRecord_kit::PageVideoRecord_kit(QWidget* p)
{
	video_displayer = new QLabel(p);
	video_displayer->setGeometry(10, 10, 1186, 885);

	bed_num_label = new QLabel(p);
	bed_num_label->setText(QStringLiteral("����ɵ��"));
	bed_num_label->setAlignment(Qt::AlignCenter);
	bed_num_label->setGeometry(1250, 10, 100, 40);
	bed_num_label->setStyleSheet("background-color: rgb(251,251,251);border:2px solid #242424;border-radius:5px;");

	patient_name_label = new QLabel(p);
	patient_name_label->setText(QStringLiteral("����ɵ��"));
	patient_name_label->setAlignment(Qt::AlignCenter);
	patient_name_label->setGeometry(1400, 10, 100, 40);
	patient_name_label->setStyleSheet("background-color: rgb(251,251,251);border:2px solid #242424;border-radius:5px;");

	video_time_display = new QLabel(p);
	video_time_display->setGeometry(1400, 300, 100, 30);
	video_time_display->setStyleSheet("background-color: rgb(251,251,251);");
	video_time_display->hide();	// ��ʼĬ�ϲ���ʾ

	record_btn = new QPushButton(p);
	//record_btn->setObjectName("pushButtonVideo");
	record_btn->setStyleSheet("background-color: rgb(0,187,163);color: #ffffff;");
	record_btn->setIcon(QIcon("./Resources/icon/PauseVideo2.png"));
	record_btn->setText(QStringLiteral("¼��"));
	record_btn->setGeometry(1500, 300, 200, 80);

	exit_btn = new QPushButton(p);
	//exit_btn->setObjectName("pushButtonVideo");
	exit_btn->setStyleSheet("background-color: rgb(0,187,163);color: #ffffff;");
	exit_btn->setIcon(QIcon("./Resources/icon/PauseVideo2.png"));
	exit_btn->setText(QStringLiteral("�����������"));
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
}

PageVideoRecord::PageVideoRecord(QWidget* parent) :
	QWidget(parent)
{
	this->setStyleSheet("background-color: rgb(251,251,251);");
	//this->setObjectName("widgetBottomShelter");

	m_PageVideoRecord_kit = new PageVideoRecord_kit(this);
	connect(m_PageVideoRecord_kit->record_btn, &QPushButton::clicked, this, &PageVideoRecord::slot_begin_or_finish_record);
	connect(m_PageVideoRecord_kit->exit_btn, &QPushButton::clicked, this, &PageVideoRecord::slot_exit);
	connect(m_PageVideoRecord_kit->video_list, &QListWidget::itemClicked, this, &PageVideoRecord::slot_replay_video);
	clear_video_displayer();

	m_video_getframe_timer = new QTimer(this);
	m_video_getframe_timer->setTimerType(Qt::TimerType::PreciseTimer);
	connect(m_video_getframe_timer, &QTimer::timeout, this, &PageVideoRecord::slot_timeout_getframe);

	m_record_duration_timer = new QTimer(this);
	m_record_duration_timer->setTimerType(Qt::TimerType::PreciseTimer);
	connect(m_record_duration_timer, &QTimer::timeout, this, &PageVideoRecord::slot_timeout_video_duration_timer);
	
	m_record_duration_period = 0;

	m_camerabase = nullptr;
	m_avt_camera = new AVTCamera;

	m_videoplayer = new VideoPlayer_ffmpeg(this);
	m_videoplayer->hide();
	m_videoplayer->setGeometry(100, 100, 1000, 1000);
}

void PageVideoRecord::prepare_record(const QString & examid)
{
	m_videoplayer->play("C:\\Users\\7723\\Desktop\\animate\\1080P_4000K_241420141.mp4");

	m_examid = examid;

	clear_video_displayer();

	// @todo �����ݿ��л�ȡ֮ǰ¼�ƹ�����Ƶ
	// m_recored_video_list = TopVDB::getRecoredVideo(examid);

	// @todo Ѱ����������� ��ʼȡ֡
	m_camerabase = m_avt_camera;
	int openCamera_res = m_camerabase->openCamera();
	switch (openCamera_res)
	{
	case AVTCamera::OpenStatus::OpenSucceed:
		m_video_getframe_timer->start(1);
		break;
	case AVTCamera::OpenStatus::OpenFailed:
		PromptBoxInst(m_PageVideoRecord_kit->video_displayer)->msgbox_go(PromptBox_msgtype::Warning, PromptBox_btntype::Confirm, QStringLiteral("�����ʧ�ܣ��볢�Լ�����Ӳ����²���"), 0, false);
		break;
	case AVTCamera::OpenStatus::NoCameras:
		PromptBoxInst(m_PageVideoRecord_kit->video_displayer)->msgbox_go(PromptBox_msgtype::Warning, PromptBox_btntype::Confirm, QStringLiteral("û���ҵ�������볢�Լ�����Ӳ����²���"), 0, false);
		break;
	case AVTCamera::OpenStatus::AlreadyRunning:
		m_video_getframe_timer->start(1);
		break;
	default:
		break;
	}
}

void PageVideoRecord::clear_video_displayer()
{
	// lzx 20200430 ��һ��deep dark fantasy��ͼƬ���뵽��Ƶ��ʾ���� ��Ϊ��ʼ��ʧ�ܡ����󱳾�
	static QPixmap pixmap;
	pixmap.scaled(m_PageVideoRecord_kit->video_displayer->size());
	pixmap.fill(Qt::black);
	m_PageVideoRecord_kit->video_displayer->setPixmap(pixmap);
}

void PageVideoRecord::slot_timeout_getframe()
{
	// @todo ��ȡһ֡
	m_camerabase->getOneFrame(&m_camera_capture_mat);

	// ͼ����ʾ
	const QPixmap* to_show_pixmap = m_PageVideoRecord_kit->video_displayer->pixmap();
	static QImage to_show_iamge;
	if (!m_camera_capture_mat.empty())
	{
		to_show_iamge.scaled(m_camera_capture_mat.cols, m_camera_capture_mat.rows);
		to_show_iamge.loadFromData(m_camera_capture_mat.data, QImage::Format_Indexed8);

		to_show_pixmap->fromImage(to_show_iamge);
	}

	// ������Ƶ
	if (m_record_duration_timer->isActive())
	{
		m_VideoWriter.write(m_camera_capture_mat);
	}
}

void PageVideoRecord::slot_begin_or_finish_record()
{
	if (!m_record_duration_timer->isActive())	// û��¼��״̬ ��ʼ¼��
	{
		m_PageVideoRecord_kit->video_time_display->show();
		m_record_duration_period = 0;
		m_PageVideoRecord_kit->video_time_display->setText("00:00:00");

		// @todo ������Ƶ���� ��ʼ����Ƶͷ
		// m_VideoWriter.open();

		m_record_duration_timer->start(1000);
	}
	else										// ֹͣ¼��
	{
		m_PageVideoRecord_kit->video_time_display->hide();
		m_record_duration_timer->stop();
		m_video_getframe_timer->stop();

		// ��������ͼ
		QListWidgetItem* new_video_thumbnail = new QListWidgetItem(m_PageVideoRecord_kit->video_list);
		new_video_thumbnail->setIcon(QIcon(QPixmap("E:/codes/TopV/V100/V2.85+/TopV2/data/image/20191231170125cut_0_60.bmp")));
		new_video_thumbnail->setText('T' + QString::number(m_PageVideoRecord_kit->video_list->count()));
		new_video_thumbnail->setTextAlignment(Qt::AlignCenter);
		m_PageVideoRecord_kit->video_list->addItem(new_video_thumbnail);

		// ������Ƶ
		m_VideoWriter.release();
	}
}

void PageVideoRecord::slot_timeout_video_duration_timer()
{
	++m_record_duration_period;
	QTime video_show_time(0, 0, 0, 0);
	video_show_time = video_show_time.addSecs(m_record_duration_period);
	m_PageVideoRecord_kit->video_time_display->setText(video_show_time.toString("hh:mm:ss"));

	// �ֻ����¼һ����
	if (video_show_time.hour() >= 1)
	{
		slot_begin_or_finish_record();
	}
}

void PageVideoRecord::slot_replay_video(QListWidgetItem* choosen_video)
{
	int idx = 0;
	for (idx = 0; idx < m_PageVideoRecord_kit->video_list->count(); ++idx)
	{
		QListWidgetItem* video_item = m_PageVideoRecord_kit->video_list->item(idx);
		if (video_item == choosen_video)
			break;
	}

	if (idx >= m_recored_videoname_list.size())
	{
		PromptBoxInst()->msgbox_go(PromptBox_msgtype::Warning, PromptBox_btntype::Confirm, QStringLiteral("�����б��ƺ�����Щ����"), 0, false);
		return;
	}
	
	const QString & to_play_video_name = m_recored_videoname_list[idx];
	// @todo ����

	m_videoplayer->play(to_play_video_name);
}

void PageVideoRecord::slot_exit()
{
	if (m_record_duration_timer->isActive())	// ¼���벻Ҫ����ҷ� �ο��¹�ϣ�¼�
	{
		PromptBoxInst()->msgbox_go(
			PromptBox_msgtype::Warning,
			PromptBox_btntype::None,
			QStringLiteral("����ֹͣ¼��"),
			1000,
			true);
		return;
	}

	// @todo �ر�����ͷ
	m_camerabase->closeCamera();

	// ����һ����ҳ��رյ��ź� �Ӳ���������ν
	emit PageVideoRecord_exit(m_examid);
}
