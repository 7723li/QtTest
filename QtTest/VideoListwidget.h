#pragma once

/*
@author : lzx
@date	: 20200428
@brief
��Ƶ����б�
*/

#include <QListWidget>
#include <QMouseEvent>
#include <QPoint>
#include <QScrollBar>

#include "AVTCamera.h"

/*
@brief
��Ƶ����б�
@note
����¼���Ҫֻ�Ǹ��ǵ��Դ�������¼� ����bug
�������ǿ��п���
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
