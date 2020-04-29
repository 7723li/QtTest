#include "PageVideoRecord.h"

PageVideoRecord_kit::PageVideoRecord_kit(QWidget* p)
{
	video_displayer = new QLabel(p);
	video_displayer->setGeometry(10, 10, 1186, 885);
	QPixmap pixmap(video_displayer->size());
	pixmap.fill(Qt::black);
	video_displayer->setPixmap(pixmap);

	bed_num_label = new QLabel(p);
	bed_num_label->setText(QStringLiteral("ÄãÊÇÉµ±Æ"));
	bed_num_label->setGeometry(1250, 10, 100, 40);

	patient_name_label = new QLabel(p);
	patient_name_label->setText(QStringLiteral("ÄãÊÇÉµŒÅ"));
	patient_name_label->setGeometry(1400, 10, 100, 40);

	video_duration_display = new QLabel(p);
	video_duration_display->setText("00:00:00");

	record_btn = new QPushButton(p);
	record_btn->setObjectName("pushButtonVideo");
	record_btn->setText(QStringLiteral("shitshitshit"));
	record_btn->setGeometry(1275, 300, 200, 80);

	finish_check_btn = new QPushButton(p);
	finish_check_btn->setObjectName("pushButtonVideo");
	finish_check_btn->setText(QStringLiteral("fuckfuckfuck"));
	finish_check_btn->setGeometry(1325, 500, 150, 30);

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
	m_PageVideoRecord_kit = new PageVideoRecord_kit(this);

	// this->setObjectName("widgetBottomShelter");

	m_video_duration_timer = new QTimer(this);
	connect(m_video_duration_timer, &QTimer::timeout, this, &PageVideoRecord::slot_video_recorded);
}

void PageVideoRecord::slot_video_recorded()
{
	++m_video_duration_realtime;
	QTime to_display(0, 0, 0);
	to_display.addSecs(m_video_duration_realtime);
	m_PageVideoRecord_kit->video_duration_display->setText(to_display.toString("hh:MM:ss"));
}
