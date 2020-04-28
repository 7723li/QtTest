#include "sth_widget.h"

sth_widget::sth_widget(QWidget* parent):
	QWidget(parent)
{
	btn_1 = new QPushButton(this);
	btn_2 = new QPushButton(this);
	view = new QTableView(this);

	btn_1->setText("1234");
	btn_2->setText("5678");

	btn_1->setGeometry(0, 0, 100, 20);
	btn_2->setGeometry(125, 0, 100, 20);
	view->setGeometry(0, 30, parent != nullptr ? parent->width() : 400, 300);
}
