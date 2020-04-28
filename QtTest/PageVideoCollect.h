#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QListWidget>

#include "PromptBox.h"
#include "VideoListwidget.h"

typedef struct PageVideoCollect_kit
{
public:
	explicit PageVideoCollect_kit(QWidget* p);
	~PageVideoCollect_kit(){}

public:
	QLabel* image_display;
	QLabel* bed_num_label;
	QLabel* patient_name_label;
	QPushButton* record_btn;
	QPushButton* finish_check_btn;
	VideListwidget* video_list;
}
PageVideoCollect_kit;

class PageVideoCollect : public QWidget
{
	Q_OBJECT

public:
	explicit PageVideoCollect(QWidget* parent);
	~PageVideoCollect(){}

public:
	PageVideoCollect_kit* m_PageVideoCollect_kit;
};
