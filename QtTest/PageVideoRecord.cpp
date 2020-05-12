#include "PageVideoRecord.h"

const QString g_test_picname = "./test.jpg";
const QString g_test_videoname = "./test.avi";

PageVideoRecord_kit::PageVideoRecord_kit(QWidget* p)
{
	frame_displayer = new FrameDisplayWidget(p);
	frame_displayer->setGeometry(10, 10, 1186, 885);

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

	videoplayer = new VideoPlayer_ffmpeg(p);
	videoplayer->hide();								// Ĭ������
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
	connect(m_PageVideoRecord_kit->video_list, &VideoListWidget::video_choosen, this, &PageVideoRecord::slot_replay_begin);
	connect(m_PageVideoRecord_kit->videoplayer, &VideoPlayer_ffmpeg::finish_play_video, this, &PageVideoRecord::slot_replay_finish);

	m_get_frame_timer = new QTimer(this);
	m_get_frame_timer->setTimerType(Qt::TimerType::PreciseTimer);
	connect(m_get_frame_timer, &QTimer::timeout, this, &PageVideoRecord::slot_get_one_frame);

	m_record_duration_timer = new QTimer(this);
	m_record_duration_timer->setTimerType(Qt::TimerType::PreciseTimer);
	connect(m_record_duration_timer, &QTimer::timeout, this, &PageVideoRecord::slot_timeout_video_duration_timer);

	m_record_duration_period = 0;

	m_avt_camera = new AVTCamera;
	m_camerabase = m_avt_camera;	// Ĭ��ʹ��AVT����ͷ
}

void PageVideoRecord::init_PageVideoRecord(const QString & examid)
{
	m_examid = examid;

	load_old_videos();				// ����֮ǰ����Ƶ
	clear_videodisplay();			// ������ʾ��
	begin_camera_and_capture();		// �����ʼ����
}

void PageVideoRecord::clear_videodisplay()
{
	// lzx 20200430 ��һ��deep dark fantasy(����)��ͼƬ���뵽��Ƶ��ʾ���� ��Ϊ��ʼ��ʧ�ܡ����󱳾�
	QSize dispalyer_size = m_PageVideoRecord_kit->frame_displayer->size();
	cv::Mat deep_dark_fantasy = cv::Mat::zeros(dispalyer_size.width(), dispalyer_size.height(), CV_8UC1);
	m_PageVideoRecord_kit->frame_displayer->show_frame(deep_dark_fantasy);
}

void PageVideoRecord::load_old_videos()
{
	// todo lzx �����ݿ��м���
	std::vector<int> index({ 0, 1, 2 });
	for (int i : index)
	{
		put_one_video_thumbnail(g_test_videoname, g_test_picname);
	}
}
void PageVideoRecord::put_one_video_thumbnail(const QString & video_path, const QString & icon_path)
{
	// ��������ͼ
	QListWidgetItem* new_video_thumbnail = new QListWidgetItem(m_PageVideoRecord_kit->video_list);
	new_video_thumbnail->setIcon(QIcon(QPixmap(icon_path)));
	new_video_thumbnail->setText('T' + QString::number(m_PageVideoRecord_kit->video_list->count()));
	new_video_thumbnail->setTextAlignment(Qt::AlignCenter);
	m_PageVideoRecord_kit->video_list->addItem(new_video_thumbnail);

	// ��¼¼�Ƶ���Ƶ����
	m_recored_videoname_list.push_back(g_test_videoname);
}

void PageVideoRecord::begin_camera_and_capture()
{
	int openstatus = m_camerabase->openCamera();			// �����
	show_camera_openstatus(openstatus);
	if (camerabase::OpenStatus::OpenSuccess != openstatus)
	{
		stop_camera_and_capture();
		return;
	}
	m_show_frame = true;
	begin_capture();
}
void PageVideoRecord::stop_camera_and_capture()
{
	m_show_frame = false;
	stop_capture();
	if (camerabase::CloseFailed == m_camerabase->closeCamera())
	{
		PromptBoxInst()->msgbox_go(PromptBox_msgtype::Failed,
			PromptBox_btntype::None,
			QStringLiteral("�ر�ʧ�ܣ��볢�԰β�\n����������ͷ"),
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
				PromptBox_msgtype::Warning, PromptBox_btntype::None, QStringLiteral("����򿪳ɹ�"), 2000, true);
		}
		else
		{
			PromptBoxInst(m_PageVideoRecord_kit->frame_displayer)->msgbox_go(
				PromptBox_msgtype::Warning, PromptBox_btntype::None, QStringLiteral("����򿪳ɹ�,�����ݴ���ʧ��"), 2000, true);
		}
	}
	else if (camerabase::OpenStatus::OpenFailed == openstatus)
	{
		PromptBoxInst(m_PageVideoRecord_kit->frame_displayer)->msgbox_go(
			PromptBox_msgtype::Warning, PromptBox_btntype::None,
			QStringLiteral("�����ʧ��\n�볢�Լ�����Ӳ����²���"), 2000, true);
	}
	else if (camerabase::OpenStatus::NoCameras == openstatus)
	{
		PromptBoxInst(m_PageVideoRecord_kit->frame_displayer)->msgbox_go(
			PromptBox_msgtype::Warning, PromptBox_btntype::None,
			QStringLiteral("û���ҵ����\n�볢�Լ�����Ӳ����²���"), 2000, true);
	}
}

void PageVideoRecord::begin_capture()
{
	m_get_frame_timer->start(1);
}
void PageVideoRecord::stop_capture()
{
	m_get_frame_timer->stop();
}

void PageVideoRecord::begin_record()
{
	m_PageVideoRecord_kit->video_time_display->show();
	m_record_duration_period = 0;
	m_PageVideoRecord_kit->video_time_display->setText("00:00:00");

	double fps = m_camerabase->get_framerate();
	int w = 0, h = 0;
	m_camerabase->get_frame_wh(w, h);
	if (m_VideoWriter.open(g_test_videoname.toStdString().c_str(), CV_FOURCC('M', 'J', 'P', 'G'), fps, cv::Size(w, h), false))
		m_record_duration_timer->start(1000);
}
void PageVideoRecord::stop_record()
{
	m_PageVideoRecord_kit->video_time_display->hide();
	m_record_duration_timer->stop();

	put_one_video_thumbnail(g_test_videoname, g_test_picname);

	m_VideoWriter.release(); // ������Ƶ
}

void PageVideoRecord::slot_get_one_frame()
{
	static cv::Mat mat;
	if (false == m_camerabase->get_one_frame(mat) ||
		mat.empty())
	{
		return;
	}

	// ��ʾ
	if (m_show_frame)
	{
		m_PageVideoRecord_kit->frame_displayer->show_frame(mat);
	}

	// д��֡
	if (m_record_duration_timer->isActive())
	{
		if (m_VideoWriter.isOpened())
		{
			m_VideoWriter.write(mat);
		}
	}
}

void PageVideoRecord::slot_begin_or_finish_record()
{
	if (!m_get_frame_timer->isActive())
	{
		PromptBoxInst(this)->msgbox_go(PromptBox_msgtype::Warning, PromptBox_btntype::None, QStringLiteral("û������ͷ��������"), 2000, true);
		return;
	}

	if (!m_record_duration_timer->isActive())
	{
		begin_record();							// û��¼��״̬ ��ʼ¼��
	}
	else
	{
		stop_record();							// ֹͣ¼��
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
		PromptBoxInst()->msgbox_go(PromptBox_msgtype::Warning, PromptBox_btntype::Confirm, QStringLiteral("�����б��ƺ�����Щ����"), 0, false);
		return;
	}
	
	const QString & to_play_video_name = m_recored_videoname_list[idx];

	m_show_frame = false;

	m_PageVideoRecord_kit->video_list->setEnabled(false);
	m_PageVideoRecord_kit->videoplayer->play(to_play_video_name);
}
void PageVideoRecord::slot_replay_finish()
{
	m_PageVideoRecord_kit->video_list->setEnabled(true);
	
	m_show_frame = true;
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

	stop_camera_and_capture();

	// ����һ����ҳ��رյ��ź� �Ӳ���������ν
	emit PageVideoRecord_exit(m_examid);
}
