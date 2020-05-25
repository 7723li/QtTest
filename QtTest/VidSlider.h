#pragma once

#include <QSlider>
#include<QPainter>
#include<QLabel>
#include<QMouseEvent>
#include<set>

/*!
*@brief
*枚举类
*/
enum class VesselQuality
{
	great,//质量非常好
	good,//质量一般
	bad//质量差
};

/*!
*@brief
*暂时结构体,存质量帧的范围
*/
struct FrameRange
{
	int beginFrameNo;
	int endFrameNo;
	VesselQuality qua;

	bool operator < (const FrameRange & s2) const 
		{
			return beginFrameNo<s2.beginFrameNo;
		}
};

using FrameRangeData = QVector<FrameRange>;

class VidSlider : public QSlider
{
	Q_OBJECT

public:

	/*!
	*@brief
	*构造函数
	*/
	VidSlider(QWidget *parent);


	/*!
	*@brief
	*析构函数
	*/
	~VidSlider();

	/*!
	*@brief
	*设置进度条的长度
	*@param[1] MaxNum 进度条长度
	*/
	void SetSliderRange(int MaxNum);

	/*!
	*@brief
	*m_qvec赋值
	*/
	void SetVesselQuality(FrameRangeData vec);

	/*!
	*@brief
	*设置当前截图帧
	*/
	void SetCurrentFramNo(int frameNo);

	/*!
	*@brief
	*设置总帧数
	*/
	void SetFrameCount(int frameCount);

	/*!
	*@brief
	*设置父视频时间
	*/
	void SetVideoTime(int time);

	/*!
	*@brief
	*依据m_qvec设置slider背景颜色
	*/
	void SetSliderColor();


	/*!
	*@brief
	*依据FrameRangeQua类型,返回相对应颜色
	*@param[1] qua 枚举类型
	*/
	QString GetColorFromFrameRangeQua(VesselQuality  qua);

	/*!
	*@brief
	*返回要截取的子视频的开始帧与结束帧
	*@param[1] begin_frame开始帧,@param[2] end_frame结束帧,
	*/
	void GetCutFrameRange(int &begin_frame, int &end_frame);


	/*!
	*@brief
	*返回要截取的子视频的开始时间与结束时间
	*@param[1] begin_frame开始时间,@param[2] end_frame结束时间,
	*/
	void GetCutTimeRange(int &begin_timer, int &end_timer);

	/*!
	*@brief
	*剪切子视频取消
	*/
	void CancleCutVideo();
	
private:
	FrameRangeData m_qvec;//存储质量帧
	int m_allFrameCount;//总帧数
	int m_videoTime;//父视频总时间
	int m_currentFrameNo;//当前帧
	QLabel * m_LeftBlade;//左侧刀片
	QLabel * m_RightBlade;//右侧刀片

	bool m_is_ready_to_cut;//标志位,为True则屏蔽滑动条的鼠标事件
	bool isLeftClicked;//左侧刀片点击了
	bool isRightClicked;//右侧刀片点击

	QSet<int> m_CutDotList;

	QVector<QLabel *> m_CutVideoArea;//存放截图区域
protected:
	/*！
	*@brief
	*截图后，绘画点
	*@param[1] QPaintEvent类型
	*/
	void paintEvent(QPaintEvent *event);

	/*！
	*@brief
	*鼠标点击事件
	*/
	void mousePressEvent(QMouseEvent *event);

	/*！
	*@brief
	*鼠标移动事件
	*/
	void mouseMoveEvent(QMouseEvent *event);

	/*！
	*@brief
	*鼠标释放事件
	*/
	void mouseReleaseEvent(QMouseEvent *event);

	/*！
	*@brief
	*过滤事件,获取左右刀片的点击
	*/
	bool eventFilter(QObject *obj, QEvent *event);

	/*！
	*@brief
	*滚轮事件
	*/
	void wheelEvent(QWheelEvent *event);
public slots:
	/*
	*@brief
	*截取视频起始步骤
	*显示质量帧范围槽函数
	*/
	void Begin_cutout_video();

	/*
	*@brief
	*截取视频结束步骤
	*隐藏质量帧范围槽函数
	*/
	void Finish_cutout_video();

signals:
	/*!
	@brief
	进度条鼠标或滚轮事件
	*/
	void slider_motivated();
	/*!
	@brief
	刀片(视频截取 起始、结束位置)移动事件
	*/
	void blade_motivated();
};
