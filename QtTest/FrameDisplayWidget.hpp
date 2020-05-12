#pragma once

#include <QPaintEvent>
#include <QPainter>
#include <QOpenGLWidget>
#include <QImage>

#include "opencv2/core/core.hpp"
#include "opencv2/core/mat.hpp"

/*
@brief
通用高性能连续图像显示控件
@note
继承自QOpenGLWidget
@interface[1] show_frame(const uchar* frame_data, int w, int h) [ 默认为单通道灰度图 可以使用 ]
@interface[2] show_frame(const cv::Mat* frame)	[ 建议使用 ]
*/
class FrameDisplayWidget : public QOpenGLWidget
{
	Q_OBJECT

public:
	FrameDisplayWidget(QWidget* p) : QOpenGLWidget(p)
	{
		m_show_mat = cv::Mat(this->height(), this->width(), CV_8UC1);
	}
	~FrameDisplayWidget(){}

public slots:
	/*
	@brief
	传入图像数据 默认为单通道灰度图
	*/
	void show_frame(uchar* frame_data, int w, int h)
	{
		if (nullptr == frame_data)
			return;

		m_show_mat = cv::Mat(h, w, CV_8UC1, frame_data);
		update();
	}

	/*
	@brief
	传入图像数据 默认为单通道灰度图
	*/
	void show_frame(cv::Mat & frame)
	{
		frame.copyTo(m_show_mat);
		update();
	}

protected:
	void paintEvent(QPaintEvent* e)
	{
		QOpenGLWidget::paintEvent(e);

		if (m_show_mat.empty())
			return;

		QImage image;
		switch (m_show_mat.type())
		{
		case CV_8UC1:
			image = QImage(m_show_mat.data, m_show_mat.cols, m_show_mat.rows, QImage::Format_Grayscale8);
			break;
		case CV_8UC3:
			image = QImage(m_show_mat.data, m_show_mat.cols, m_show_mat.rows, QImage::Format_RGB888);
			break;
		case CV_8UC4:
			image = QImage(m_show_mat.data, m_show_mat.cols, m_show_mat.rows, QImage::Format_RGB32);
			break;
		default:
			break;
		}

		if (image.isNull())
			return;

		QPainter painter;
		painter.begin(this);
		painter.drawImage(QPoint(0, 0), image);
	}

private:
	cv::Mat m_show_mat;
};
