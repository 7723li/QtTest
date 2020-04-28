#pragma once

#include <QWidget>
#include <QPushButton>
#include <QTableView>
#include <QLabel>

class sth_widget : public QWidget
{
public:
	explicit sth_widget(QWidget* parent = nullptr);
	~sth_widget(){}

	QPushButton* btn_1;
	QPushButton* btn_2;
	QTableView* view;
};