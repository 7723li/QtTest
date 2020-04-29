#pragma once

/*
@author : lzx
@date	: 20200428
@brief
视频存放列表
*/

#include <QListWidget>
#include <QMouseEvent>
#include <QPoint>
#include <QScrollBar>

/*
@brief
视频存放列表
@note
鼠标事件主要只是覆盖掉自带的鼠标事件 会有bug
其他都是可有可无
*/
class VideoListWidget : public QListWidget
{
public:
	explicit VideoListWidget(QWidget* p);
	~VideoListWidget();

protected:
	void mousePressEvent(QMouseEvent* e) override;
	void mouseMoveEvent(QMouseEvent* e) override;
	void mouseReleaseEvent(QMouseEvent* e) override;

private:
	bool m_is_left_clicked;
	QPoint m_left_clicked_pos;
};
