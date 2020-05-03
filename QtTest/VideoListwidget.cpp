#include "VideoListWidget.h"

VideoListWidget::VideoListWidget(QWidget* p):
QListWidget(p)
{
	m_is_left_clicked = false;
	m_left_clicked_pos = this->pos();
}

VideoListWidget::~VideoListWidget()
{

}

void VideoListWidget::mousePressEvent(QMouseEvent* e)
{
	if (e->button() != Qt::MouseButton::LeftButton)
		return;

	m_is_left_clicked = true;
	m_left_clicked_pos = e->pos();
}

void VideoListWidget::mouseMoveEvent(QMouseEvent* e)
{
	if (!m_is_left_clicked)
		return;

	this->horizontalScrollBar()->setValue(m_left_clicked_pos.x() - e->x());
}

void VideoListWidget::mouseReleaseEvent(QMouseEvent* e)
{
	m_is_left_clicked = false;

	QListWidgetItem* choosen_widget = this->itemAt(e->pos());

	emit video_choosen(choosen_widget);
}
