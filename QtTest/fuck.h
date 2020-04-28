#ifndef FUCK_H
#define FUCK_H

#include <QtWidgets/QWidget>
#include <QMessagebox>
#include <QDesktopWidget>
#include <QStackedWidget>

#include "ui_fuck.h"

#include "PromptBox.h"
#include "sth_widget.h"
#include "PageVideoCollect.h"	

#ifdef _WIN32
#include <windows.h>
#endif

class Fuck : public QWidget
{
	Q_OBJECT

public:
	Fuck(QWidget *parent = 0);
	~Fuck();

private:
	Ui::FuckClass ui;

	QPushButton* btn_1;
	QPushButton* btn_2;
	QPushButton* btn_3;
	QTimer* timer_useless;
	QEventLoop* loop_useless;
	bool running;
	QRect m_using_resolution;

	QStackedWidget* m_stackwidget;
	sth_widget* sth_widg;
	PageVideoCollect* m_PageVideoCollect;

private slots:
	void slot_1();
	void slot_finish_loop();
	void slot_resolution_resize(int);
	void slot_switch_sth_widg();
	void slot_switch_PageVideoCollect();
};

#endif // FUCK_H
