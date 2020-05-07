#include "PageVideoRecord.h"

const QString g_test_picname = "./test.jpg";
const QString g_test_videoname = "./test.avi";

PageVideoRecord_kit::PageVideoRecord_kit(QWidget* p)
{
	video_displayer = new VideoDisplayWidget(p);
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
	connect(m_PageVideoRecord_kit->video_list, &VideoListWidget::video_choosen, this, &PageVideoRecord::slot_replay_recordedvideo);

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

void PageVideoRecord::prepare_record(const QString & examid)
{
	m_examid = examid;

	clear_video_displayer();

	/* @todo �����ݿ��л�ȡ֮ǰ¼�ƹ�����Ƶ */
	// m_recored_video_list = TopVDB::getRecoredVideo(examid);

	int w = 0, h = 0;
	int closeCam_res = m_camerabase->closeCamera();		// ��ǰ�ȹر� ������� ����֮ǰ�д��������������
	int openCamera_res = m_camerabase->openCamera();	// �����
	switch (openCamera_res)
	{
	case camerabase::OpenStatus::OpenSuccess:
		m_camerabase->get_frame_wh(w, h);				// ���������������ݵ�֡
		if (!m_mat.empty())
			m_mat.release();
		m_mat = cv::Mat(h, w, CV_8UC1);
		m_get_frame_timer->start(1);					// ������ʱ�� ��ʼ��ȡ֡����ʾ ����Ƶ��1s/1ms=1000Hz ��������

		// ��ʾ�ɹ�������ȴ�2�� �ȴ���������������ȶ�Ȼ����Ի�ȡ�Ƚ�׼ȷ��fps��ֵ ������д����Ƶ
		PromptBoxInst(m_PageVideoRecord_kit->video_displayer)->msgbox_go(PromptBox_msgtype::Warning, PromptBox_btntype::None, QStringLiteral("����򿪳ɹ�"), 2000, true);
		break;
	case camerabase::OpenStatus::OpenFailed:
		PromptBoxInst(m_PageVideoRecord_kit->video_displayer)->msgbox_go(PromptBox_msgtype::Warning, PromptBox_btntype::None, QStringLiteral("�����ʧ�ܣ��볢�Լ�����Ӳ����²���"), 2000, true);
		break;
	case camerabase::OpenStatus::NoCameras:
		PromptBoxInst(m_PageVideoRecord_kit->video_displayer)->msgbox_go(PromptBox_msgtype::Warning, PromptBox_btntype::None, QStringLiteral("û���ҵ�������볢�Լ�����Ӳ����²���"), 2000, true);
		break;
	default:
		break;
	}
}

void PageVideoRecord::clear_video_displayer()
{
	// lzx 20200430 ��һ��deep dark fantasy��ͼƬ���뵽��Ƶ��ʾ���� ��Ϊ��ʼ��ʧ�ܡ����󱳾�
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

	// ��ʾ
	if (!m_mat.empty())
	{
		m_PageVideoRecord_kit->video_displayer->show_frame(&m_mat);
		m_PageVideoRecord_kit->video_displayer->update();
	}

	// д��֡
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
		PromptBoxInst()->msgbox_go(PromptBox_msgtype::Warning, PromptBox_btntype::None, QStringLiteral("û������ͷ��������"), 2000, true);
		return;
	}

	if (!m_record_duration_timer->isActive())	// û��¼��״̬ ��ʼ¼��
	{
		m_PageVideoRecord_kit->video_time_display->show();
		m_record_duration_period = 0;
		m_PageVideoRecord_kit->video_time_display->setText("00:00:00");

		if(m_VideoWriter.open(g_test_videoname.toStdString().c_str(), CV_FOURCC('M', 'J', 'P', 'G'), 120, cv::Size(m_mat.cols, m_mat.rows), false))
			m_record_duration_timer->start(1000);
	}
	else										// ֹͣ¼��
	{
		m_PageVideoRecord_kit->video_time_display->hide();
		m_record_duration_timer->stop();

		// ��������ͼ
		QListWidgetItem* new_video_thumbnail = new QListWidgetItem(m_PageVideoRecord_kit->video_list);
		new_video_thumbnail->setIcon(QIcon(QPixmap(g_test_picname)));
		new_video_thumbnail->setText('T' + QString::number(m_PageVideoRecord_kit->video_list->count()));
		new_video_thumbnail->setTextAlignment(Qt::AlignCenter);
		m_PageVideoRecord_kit->video_list->addItem(new_video_thumbnail);

		// ����¼�Ƶ���Ƶ����
		m_recored_videoname_list.push_back(g_test_videoname);

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
		PromptBoxInst()->msgbox_go(PromptBox_msgtype::Warning, PromptBox_btntype::Confirm, QStringLiteral("�����б��ƺ�����Щ����"), 0, false);
		return;
	}
	
	const QString & to_play_video_name = m_recored_videoname_list[idx];
	// @todo ����

	m_PageVideoRecord_kit->videoplayer->play(to_play_video_name);
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

	do
	{
		m_get_frame_timer->stop();
	}
	while (m_get_frame_timer->isActive());
	int close_status = m_camerabase->closeCamera();

	// ����һ����ҳ��رյ��ź� �Ӳ���������ν
	emit PageVideoRecord_exit(m_examid);
}
