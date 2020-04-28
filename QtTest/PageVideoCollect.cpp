#include "PageVideoCollect.h"

PageVideoCollect_kit::PageVideoCollect_kit(QWidget* p)
{
	image_display = new QLabel(p);
	image_display->setGeometry(10, 10, 1186, 885);
	QPixmap pixmap(image_display->size());
	pixmap.fill(Qt::black);
	image_display->setPixmap(pixmap);

	bed_num_label = new QLabel(p);
	bed_num_label->setText(QStringLiteral("ÄãÊÇÉµ±Æ"));
	bed_num_label->setGeometry(1250, 10, 100, 40);

	patient_name_label = new QLabel(p);
	patient_name_label->setText(QStringLiteral("ÄãÊÇÉµŒÅ"));
	bed_num_label->setGeometry(1300, 10, 100, 40);

	record_btn = new QPushButton(p);
	record_btn->setObjectName("pushButtonVideo");
	record_btn->setText(QStringLiteral("shitshitshit"));
	record_btn->setGeometry(1275, 500, 200, 80);

	finish_check_btn = new QPushButton(p);
	finish_check_btn->setText(QStringLiteral("fuckfuckfuck"));
	finish_check_btn->setGeometry(1325, 700, 150, 30);

	video_list = new QScrollArea(p);
	video_list->setAlignment(Qt::AlignHCenter);
	video_list->setGeometry(1196, 800, p->width() - 1196, 300);
	video_list->
}

PageVideoCollect::PageVideoCollect(QWidget* parent) :
	QWidget(parent)
{
	m_PageVideoCollect_kit = new PageVideoCollect_kit(this);
}
