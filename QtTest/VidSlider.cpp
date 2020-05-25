#include "VidSlider.h"

VidSlider::VidSlider(QWidget *parent)
	: QSlider(parent), m_allFrameCount(0), m_currentFrameNo(0), m_is_ready_to_cut(false), isLeftClicked(false), isRightClicked(false)
{	

	//#1 下列注释代码是一个样式实例，用于设置滑动条背后的颜色
	/*this->setStyleSheet(
	"QSlider::groove:horizontal {height: 10; background: qlineargradient(x1 : 0, y1 : 0, x2 : 1, y2 : 0,stop : 0.399 rgb(255, 0, 0), stop : 0.4 rgb(0, 255, 0), stop : 0.799 rgb(0, 255, 0), stop : 0.8 rgb(0, 0, 255),stop : 0.89 rgb(0, 0, 255),stop : 0.99 rgb(0, 255, 255), stop : 1 rgb(0, 0, 255));}"
	"QSlider::handle:horizontal{width:12px;background-color:rgb(255,154,60);margin:-2px 0px -2px 0px;border-radius:6px;}");*/
	
	this->setStyleSheet(
		"QSlider::groove:horizontal {height: 8;background-color: rgb(188,188,188);}"
		"QSlider::sub-page:horizontal{background-color:rgb(255,173,104);}"
		"QSlider::handle:horizontal{width:12px;background-color:rgb(255,154,60);margin:-2px 0px -2px 0px;border-radius:6px;}");
	
	m_LeftBlade = new QLabel(this);
	m_LeftBlade->hide();
	m_LeftBlade->setGeometry(0, 0, 8, 14);
	m_LeftBlade->installEventFilter(this);


	m_RightBlade = new QLabel(this);
	m_RightBlade->hide();
	m_RightBlade->setGeometry(this->width(), 0, 8, 14);
	m_RightBlade->installEventFilter(this);




}

VidSlider::~VidSlider()
{
}

void VidSlider::SetSliderRange(int MaxNum)
{
	this->setRange(0, MaxNum);//设置滑动条范围
}


void VidSlider::SetVesselQuality(FrameRangeData vec)
{
	m_qvec = vec;

}

void VidSlider::SetCurrentFramNo(int frameNo)
{
	m_CutDotList.insert(frameNo);
}

void VidSlider::SetFrameCount(int frameCount)
{
	m_allFrameCount = frameCount;
}

void VidSlider::SetVideoTime(int time)
{
	m_videoTime = time;
}

QString VidSlider::GetColorFromFrameRangeQua(VesselQuality  qua)
{
	QString ColorStr;

	//依据枚举类型,返回相应的颜色
	switch (qua)
	{
	case VesselQuality::great://非常好,暂定为红色
		ColorStr = "rgb(255, 0, 0)";
		break;
	case VesselQuality::good://好,暂定为绿色
		ColorStr = "rgb(0, 255, 0)";
		break;
	case VesselQuality::bad://垃圾,暂定为蓝色
		ColorStr = "rgb(0, 0, 255)";
		break;
	default:
		break;
	}

	return ColorStr;
}

void VidSlider::GetCutFrameRange(int &begin_frame, int &end_frame)
{
	//开始帧
	int leftpos = m_LeftBlade->pos().x() + m_LeftBlade->width();
	double per = (double)leftpos / this->width();
	begin_frame = per*m_allFrameCount;

	//结束帧
	per = (double)m_RightBlade->pos().x() / this->width();
	end_frame = per*m_allFrameCount;
}


void VidSlider::GetCutTimeRange(int &begin_timer, int &end_timer)
{
	//开始时间
	int leftpos = m_LeftBlade->pos().x() + m_LeftBlade->width();
	double per = (double)leftpos / this->width();
	begin_timer = per*m_videoTime;

	//结束时间
	per = (double)m_RightBlade->pos().x() / this->width();
	end_timer = per*m_videoTime;
}

void VidSlider::CancleCutVideo()
{	

	if (m_CutVideoArea.size() > 0)
	{
		QLabel * tmp = m_CutVideoArea.at(m_CutVideoArea.size() - 1);
		tmp->hide();

		m_CutVideoArea.pop_back();

		if (tmp)
		{
			delete tmp;
			tmp = nullptr;
		}
	}

}

void VidSlider::SetSliderColor()
{
	


	//使用渐变色进行改变颜色,取巧,问题：可能会造成字符串很长
	double preIndexStopPos = 0;//上一次的停止位

	QString allstr("QSlider::groove:horizontal {height: 8px; background: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0,");//渐变色,也就是背景色的开头

	for (int i = 0; i < m_qvec.size(); i++)//循环范围帧
	{
		double beginpos = (double)m_qvec.at(i).beginFrameNo / m_allFrameCount;//获取开始帧百分比
		double endpos = (double)m_qvec.at(i).endFrameNo / m_allFrameCount;//获取结束帧百分比

		QString Color = GetColorFromFrameRangeQua(m_qvec.at(i).qua);//依据帧质量,返回相对应的颜色

		double prepos = beginpos + 0.0001;//添加0.0001是将两个颜色之间的颜色渐变压缩在一个很小的范围内，让人肉眼无法分辨
		beginpos = beginpos + 0.0002;//0.0002是在0.0001的基础上再加0.0001,理由如上

		if (i == 0)//循环第一下,将之前的段落颜色设为rgb(188,188,188),无论帧是否从0开始,会覆盖之前的颜色
		{
			allstr += QString("stop :%1 rgb(188,188,188),stop:%2 %3 ,stop: %4 %5").arg(QString::number(prepos)).arg(QString::number(beginpos)).arg(Color)\
				.arg(QString::number(endpos)).arg(Color);

			preIndexStopPos = endpos;//存储当前帧,用于下一帧过渡
		}
		else
		{
			double preendpos = preIndexStopPos;//获取前一帧
			preendpos += 0.0001;//加0.0001过渡
			allstr += QString(",stop :%1 %2,stop:%3 %4").arg(QString::number(preendpos)).arg(Color)\
				.arg(QString::number(endpos)).arg(Color);
			preIndexStopPos = endpos;//存储当前帧,用于下一帧过渡
		}
		if (i != m_qvec.size() - 1)//不等于末尾
		{
			double nextbeginpos = (double)m_qvec.at(i + 1).beginFrameNo / m_allFrameCount;//获取下一帧
			if (nextbeginpos == endpos)//如果下一帧开始等于当前帧结束则重新开始循环
			{
				continue;
			}
			else//如果不是,就需要补颜色rgb(188,188,188),在当前帧的结束到下一帧的开始
			{
				endpos += 0.0001;//过渡
				allstr += QString(",stop :%1 rgb(188,188,188),stop:%2 rgb(188,188,188)").arg(QString::number(endpos)).arg(QString::number(nextbeginpos));
			}

			preIndexStopPos = nextbeginpos;//存储,用于下一帧过渡

			//allstr += ",";
		}
	}

	if (preIndexStopPos < 1)//如果最后结束帧小于总帧数,则补颜色rgb(188,188,188)
	{
		preIndexStopPos += 0.0001;
		allstr += QString(",stop:%1 rgb(188,188,188)").arg(QString::number(preIndexStopPos));

		allstr += QString(",stop:1 rgb(188, 188, 188));}");
	}
	else//不补颜色rgb(188,188,188)
	{
		allstr += QString(");}");

	}

	//设置滑块样式
	allstr += QString("QSlider::handle:horizontal{width:12px;background-color:rgb(255,154,60);margin:-2px 0px -2px 0px;border-radius:6px;}");
	this->setStyleSheet(allstr);


}


void VidSlider::paintEvent(QPaintEvent *event)
{	
	QSlider::paintEvent(event);

	//下述为截图的打点功能
	QPainter painter(this);

	QPen pen(QColor(0, 187, 163));

	painter.setBrush(QBrush(QColor(0, 187, 163, 150)));

	painter.setPen(pen);

	QSet<int>::iterator it;
	for (it = m_CutDotList.begin(); it != m_CutDotList.end(); ++it)
	{
		double dotPos = (double)m_currentFrameNo / m_allFrameCount;
		int sliderMax = this->width();

		dotPos = dotPos*sliderMax;

		painter.drawEllipse(QRect(dotPos, 0, 10, 10));

	}

	if (m_is_ready_to_cut)
	{	//绘画左侧刀片
		QImage Leftp(m_LeftBlade->size(),QImage::Format_ARGB32);
		//m_LeftLabel->setStyleSheet("background:color rbg(255,255,255);");
		Leftp.fill(QColor(255, 255, 255, 20));
		QPainter Leftpainter(&Leftp);
		Leftpainter.setBrush(QBrush(QColor(215, 215, 215, 80)));
		QPointF handleLeft_1(0, 0);
		QPointF handleLeft_2(7, 0.5*height());
		QPointF handleLeft_3(0, height());


		QVector<QPointF> leftPointVec;
		leftPointVec.push_back(handleLeft_1);
		leftPointVec.push_back(handleLeft_2);
		leftPointVec.push_back(handleLeft_3);


		Leftpainter.drawPolygon(leftPointVec);
		m_LeftBlade->setPixmap(QPixmap::fromImage(Leftp));
		m_LeftBlade->show();


		//绘制右侧刀片
		QImage Rightp(m_LeftBlade->size(), QImage::Format_ARGB32);
		//m_LeftLabel->setStyleSheet("background:color rbg(255,255,255);");
		Rightp.fill(QColor(255, 255, 255, 20));
		QPainter Rightpainter(&Rightp);
		Rightpainter.setBrush(QBrush(QColor(215, 215, 215, 80)));
		QPointF handleRight_1(7, 0);
		QPointF handleRight_2(0, 0.5*height());
		QPointF handleRight_3(7, height());


		QVector<QPointF> rightPointVec;
		rightPointVec.push_back(handleRight_1);
		rightPointVec.push_back(handleRight_2);
		rightPointVec.push_back(handleRight_3);


		Rightpainter.drawPolygon(rightPointVec);
		m_RightBlade->setPixmap(QPixmap::fromImage(Rightp));
		m_RightBlade->show();


		/*绘制临时的截图区域
		
				int backwidth = m_RightLabel->pos().x() - m_LeftLabel->pos().x() - m_RightLabel->width();
				int backheight = m_LeftLabel->height();
				int backpos = m_LeftLabel->pos().x() + m_LeftLabel->width();
		
				QImage area(QSize(backwidth, backheight), QImage::Format_ARGB32);
				QPainter Areapainter(&area);
				area.fill(QColor(188, 188, 188, 40));
				Areapainter.setBrush(QBrush(QColor(188, 188, 188, 40)));
				Areapainter.drawRect(-1, 0, backwidth + 1, m_LeftLabel->height());
		
				if (m_CutVideoArea.size() > 0)
				{
					m_CutVideoArea.at(m_CutVideoArea.size() - 1)->setPixmap(QPixmap::fromImage(area));
		
					m_CutVideoArea.at(m_CutVideoArea.size() - 1)->setGeometry(backpos, 2, backwidth, 8);
		
					m_CutVideoArea.at(m_CutVideoArea.size() - 1)->show();
				}*/


	}
}

bool VidSlider::eventFilter(QObject *obj, QEvent *event)
{


	if (obj == m_LeftBlade)
	{
		if (event->type() != QEvent::MouseButtonPress)
		{
			return false;
		}
		QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
		QPoint mousePos = mouseEvent->pos();

		if (m_LeftBlade->rect().contains(mousePos))
		{
			isLeftClicked = true;
		}
	}
	else if (obj == m_RightBlade)
	{
		if (event->type() != QEvent::MouseButtonPress)
		{
			return false;
		}
		QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
		QPoint mousePos = mouseEvent->pos();

		if (m_RightBlade->rect().contains(mousePos))
		{
			isRightClicked = true;
		}
	}

	return QObject::eventFilter(obj, event);
}


void VidSlider::wheelEvent(QWheelEvent *event)
{
	QSlider::wheelEvent(event);
	if (m_is_ready_to_cut)//剪切功能启动,应该改为正在播放
	{
		double Leftper = m_LeftBlade->pos().x() + m_LeftBlade->width();
		Leftper = (double)Leftper / this->width();
		int Leftvalue = Leftper*(this->maximum() - this->minimum()) + this->minimum();


		if (this->value() < Leftvalue)//判别滑块位置,不小于左刀片
		{
			this->setValue(Leftvalue);
			emit slider_motivated();
			return;
		}

		double Rightper = (double)m_RightBlade->pos().x() / this->width();
		int Rightvalue = Rightper*(this->maximum() - this->minimum()) + this->minimum();

		if (this->value() > Rightvalue)//判别滑块位置,不小于左刀片
		{
			this->setValue(Rightvalue);
			emit slider_motivated();
			return;
		}

	}

}

void VidSlider::mousePressEvent(QMouseEvent *event)
{	
	if (isLeftClicked || isRightClicked)//如果左侧刀片已点击
	{
		return;
	}

	QSlider::mousePressEvent(event);

	if (m_is_ready_to_cut)//剪切功能启动,应该改为正在播放
	{
		
		int sliderpos = event->pos().x();
		double posper = (double)sliderpos / this->width();


		int posvalue = posper*(this->maximum()) + this->minimum();

		double Leftper = m_LeftBlade->pos().x() + m_LeftBlade->width();
		Leftper = (double)Leftper / this->width();
		int Leftvalue = Leftper*(this->maximum()) + this->minimum();


		if (posvalue < Leftvalue)//判别滑块位置,不小于左刀片
		{
			this->setValue(Leftvalue);
			emit slider_motivated();
			return;
		}

		double Rightper = (double)m_RightBlade->pos().x() / this->width();
		int Rightvalue = Rightper*(this->maximum());

		if (posvalue > Rightvalue)//判别滑块位置,不小于左刀片
		{
			this->setValue(Rightvalue);
			emit slider_motivated();
			return;
		}
		
	}

	double per = (double)event->pos().x() *1.0 / this->width();
	int value = per*(this->maximum() - this->minimum()) + this->minimum();
	this->setValue(value);
	emit slider_motivated();

}
void VidSlider::mouseMoveEvent(QMouseEvent *event)
{	
	if (isLeftClicked)//左侧刀片已点击
	{	
		if (event->pos().x() <= 0)
		{
			m_LeftBlade->setGeometry(0, 0, 8, 14);
			emit blade_motivated();
		}
		else if (m_LeftBlade->pos().x() >=m_RightBlade->pos().x())//小于右侧刀片
		{
			m_LeftBlade->setGeometry(m_RightBlade->pos().x() - m_RightBlade->width(), 0, 8, 14);
			emit blade_motivated();
		}
		else
		{
			m_LeftBlade->setGeometry(event->pos().x(), 0, 8, 14);
			emit blade_motivated();
		}
	}
	else if (isRightClicked)
	{
		if (event->pos().x() >= this->width())
		{
			m_RightBlade->setGeometry(this->width() - 7, 0, 8, 14);
			emit blade_motivated();
		}
		else if (m_LeftBlade->pos().x() >= m_RightBlade->pos().x())//小于右侧刀片
		{
			m_RightBlade->setGeometry(m_LeftBlade->pos().x() + m_LeftBlade->width(), 0, 8, 14);
			emit blade_motivated();
		}
		else
		{
			m_RightBlade->setGeometry(event->pos().x(), 0, 8, 14);
			emit blade_motivated();
		}
	}

	QSlider::mouseMoveEvent(event);

	if (m_is_ready_to_cut)//剪切功能启动,应该改为正在播放
	{
		double Leftper = m_LeftBlade->pos().x() + m_LeftBlade->width();
		Leftper = (double)Leftper / (double)this->width();

		int Leftvalue = Leftper*(this->maximum());

		if (this->value() <= Leftvalue)//判别滑块位置,不小于左刀片
		{
			this->setValue(Leftvalue);
			emit slider_motivated();
			return;
		}

		double Rightper = (double)m_RightBlade->pos().x() / (double)this->width();
		int Rightvalue = Rightper*(this->maximum());

		if (this->value() >= Rightvalue)//判别滑块位置,不小于左刀片
		{
			this->setValue(Rightvalue);
			emit slider_motivated();
			return;
		}
	}

}
void VidSlider::mouseReleaseEvent(QMouseEvent *event)
{	
	if (isLeftClicked || isRightClicked)//左侧刀片已点击
	{	
		isLeftClicked = false;
		isRightClicked = false;
		return;
	}

	QSlider::mouseReleaseEvent(event);

	if (m_is_ready_to_cut)//剪切功能启动,应该改为正在播放
	{
		if (isLeftClicked)
		{	
			double Leftper = m_LeftBlade->pos().x() + m_LeftBlade->width();
			Leftper = (double)Leftper / this->width();
			int Leftvalue = Leftper*(this->maximum());

			if (this->value() <= Leftvalue)//判别滑块位置,不小于左刀片
			{
				this->setValue(Leftvalue);
				emit slider_motivated();
				return;
			}
		}
		else if (isRightClicked)
		{
			double Rightper = (double)m_RightBlade->pos().x() / this->width();
			int Rightvalue = Rightper*(this->maximum());

			if (this->value() >= Rightvalue)//判别滑块位置,不小于右侧刀片
			{
				emit slider_motivated();
				this->setValue(Rightvalue);
				return;
			}
		}

	}


}


void VidSlider::Begin_cutout_video()
{
	//SetSliderColor();//显示
	m_LeftBlade->show();
	m_RightBlade->show();

	/*QLabel *tmp = new QLabel(this);
	m_LeftLabel->hide();
	m_LeftLabel->setGeometry(0, 0, 8, 14);
	m_CutVideoArea.push_back(tmp);*/

	m_is_ready_to_cut = true;
	repaint();
}

void VidSlider::Finish_cutout_video()
{
	this->setStyleSheet(
		"QSlider::groove:horizontal {height: 8;background-color: rgb(188,188,188);}"
		"QSlider::sub-page:horizontal{background-color:rgb(255,173,104);}"
		"QSlider::handle:horizontal{width:12px;background-color:rgb(255,154,60);margin:-2px 0px -2px 0px;border-radius:6px;}");

	m_is_ready_to_cut = false;
	m_LeftBlade->hide();
	m_RightBlade->hide();
	repaint();
}