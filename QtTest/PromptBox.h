#pragma once

/*
@author : lzx
@date	: 20200420
@brief
弹框提示 兼容普通进度条 以及 批量导出的进度条
@property
弹框返回类型(PromptBox_msgtype)
弹框按钮类型(PromptBox_btntype)
进度条类型(Prompt_progbar_type)
弹框工具箱(PromptBox_kit)
弹框本体(PromptBox)
@document
..\document\Design\ModuleDesign\#14#15提示框&&进度条\#14#15提示框&&进度条验证文档.md
*/

#include <map>
#include <vector>
#include <QProgressBar>
#include <QPushButton>
#include <QLabel>
#include <QDialog>
#include <QEventLoop>
#include <QLoggingCategory>
#include <QMouseEvent>
#include <QPoint>
#include <QTimer>
#include <QObject>
#include <QCloseEvent>
#include <QApplication>
#include <QStackedWidget>

#ifdef _WIN32
#include <windows.h>
#endif

typedef const QString& tiptype;

#define PromptBoxInst PromptBox::inst

/*
@brief
弹框消息类型
@param[1] Information = 0	提醒
@param[2] Warning = 1		警告
@param[3] Question = 2		疑问
@param[4] Succeed = 3		成功
@param[5] Failed = 4		失败
*/
typedef enum class PromptBox_msgtype
{
	Information = 0,
	Warning,
	Question,
	Succeed,
	Failed,
}
PromptBox_msgtype;

/*
@brief
弹框按钮类型
@param[1] Nothing = 0					什么都没有
@param[2] Confirm = 1					仅确认
@param[3] Cancle = 2					仅取消
@param[4] Confirm_and_Cancle = 1 | 2	确认和取消
*/
typedef enum class PromptBox_btntype
{
	None = 0,
	Confirm,
	Cancle,
	Confirm_and_Cancle = Cancle | Confirm,
}
PromptBox_btntype;

/*
@brief
弹框返回类型
@param[1] Error_re = -1		错误操作
@param[2] Closed = 0		关闭
@param[3] Cancled = 1		取消
@param[4] Confirmed = 2		确认
*/
typedef enum class PromptBox_rettype
{
	Error = -1,
	Closed,
	Cancled,
	Confirmed,
}
PromptBox_rettype;

typedef enum class Prompt_progbar_type
{
	Normal_Progbar = 0,
	Batch_Progbar
}
Prompt_progbar_type;

/*
@brief
弹框工具箱
*/
class PromptBox;
typedef struct PromptBox_kit
{
public:
	explicit PromptBox_kit(PromptBox* p);
	~PromptBox_kit(){}

public:
	QWidget* background_main;			// 最底层的灰色背景
	QWidget* background_tips_progbar;	// 承载文字提示和进度条的白色背景
	QWidget* backgound_cancle_confirm;	// 承载取消、确认按钮的灰白色背景

	QPushButton* close_btn;				// 关闭按钮
	QPushButton* cancle_btn;			// 取消按钮
	QPushButton* confirm_btn;			// 确认按钮

	QStackedWidget* m_using_kitlist;	// lzx 20200430 使用容器栈代替伪迭代器

	struct _msgbox_kitlist : public QWidget
	{
		QLabel* _icon;					// 显示图标
		QLabel* tips;					// 提示框文字
	}
	*msgbox_kitlist;

	struct _progbar_kitlist : public QWidget
	{
		QProgressBar* progbar;			// 进度条
		QLabel* progbar_schedule;		// 进度条进度提示
		QLabel* tips;					// 普通进度条提示
	}
	*progbar_kitlist;

	struct _batchprogbar_kitlist : public QWidget
	{
		QProgressBar* progbar;			// 进度条
		QLabel* tip_exporting;			// 批量导出"正在导出"
		QLabel* tip_finished;			// 批量导出"已完成x%"
		QLabel* tip_exportnum;			// 批量导出"导出病例x个"
		QLabel* tip_residuetime;		// 批量导出"剩余时间"
	}
	*batchprogbar_kitlist;
}
PromptBox_kit;

/*
@brief
弹框本体
@note
单例模式
*/
class PromptBox : public QWidget
{
	Q_OBJECT

private:
	explicit PromptBox();
	~PromptBox();

public:
	// 获取实例
	static PromptBox* inst(QWidget* p = nullptr);

	/*
	@brief
	调用消息提示框
	@param[1] msgtype		显示的消息类型		PromptBox_msgtype
	@param[2] btntype		显示的按键类型		PromptBox_btntype
	@param[3] tips			文字提示				QString
	@param[4] interval_secs	阻塞时间	(s)			int
	@param[5] auto_close	自动关闭				bool
	*/
	PromptBox_rettype msgbox_go(PromptBox_msgtype msgtype, PromptBox_btntype btntype, tiptype show_tips, int interval_ms = 0, bool auto_close = false);

	/*
	@brief
	设置进度条参数
	@note
	必须先调用这个 再调用数值变化(指progbar_go)
	当然 也可以随便乱搞 只是可能不会有结果
	@param[1] min			最小值			int
	@param[2] max			最大值			int
	@param[3] bartype		进度条样式		Prompt_progbar_type
	@param[4] auto_close	自动关闭进度条	bool
	@note
	进度条的自动关闭指走完后马上就关闭
	*/
	void progbar_prepare(int min, int max, Prompt_progbar_type bartype, bool auto_close = false);
	/*
	@brief
	进度条数值变化
	@param[1] val	走动数值								int
	@param[2] tip_1 普通进度条提示 & 批量导出"正在导出"	QString
	@param[3] tip_2 批量导出"已完成x%"					QString
	@param[4] tip_3 批量导出"导出病例x个"					QString
	@param[5] tip_4 批量导出"剩余时间"					QString
	*/
	void progbar_go(int val, tiptype tip_1);
	void progbar_go(int val, tiptype tip_1, tiptype tip_2, tiptype tip_3, tiptype tip_4);

	/*
	@brief
	进度条动画
	@note
	一只肥(fake)狗(go) 可以给人缓解情绪
	只是一个动画，主线程阻塞卡顿时可以调用
	动画时间(单位:ms) t = (max - min)  interval
	@param[1] min			最小值			int
	@param[2] max			最大值			int
	@param[3]interval		阻塞时间(ms)		int
	@param[4] tips			文字提示			QString
	@param[5] autoclose		自动关闭进度条	bool
	*/
	void progbar_fakego(int min, int max, int interval_ms, QString tip, bool auto_close = true);

protected:
	void mousePressEvent(QMouseEvent* e);	// 鼠标事件 用于弹框移动
	void mouseMoveEvent(QMouseEvent* e);	// 鼠标事件 用于弹框移动
	void mouseReleaseEvent(QMouseEvent* e);	// 鼠标事件 用于弹框移动

	void closeEvent(QCloseEvent* e);		// 关闭事件 关掉东西

private:
	void set_MsgBox_style(PromptBox_msgtype msgtype, PromptBox_btntype btntype, QString show_tip);// 设置普通弹框模式
	void set_Progress_style();	// 设置进度条模式
	void set_BatchProg_style();	// 设置批量导出的进度条模式

	void stop(bool close_ = true);	// 完结 主要用于进度条

signals:
	void Prompt_probar_curval(int val);	// 进度条当前进度
	void Prompt_probar_finish();		// 进度条完结信号 只在stop函数中发射

	private slots:
	void on_kitbtn_clicked();		// 接收 任意按钮点击
	void on_progtimer_timeout();	// 接收 进度条定时器计数

private:
	static PromptBox* s_inst;		// 实例

	QEventLoop* m_loop;				// 事件循环 主要用于普通弹框
	QTimer* m_msgbox_timer;			// 弹框定时器 用于弹框自动关闭
	QTimer* m_progbar_timer;		// 进度条定时器 用于查看进度条当前数值(不用于自动关闭)
	PromptBox_kit* m_kit;			// 套件
	QProgressBar* m_using_bar;		// 有两条进度条 选一条

	bool m_prog_autoclose;			// 进度条自动关闭
	int m_prog_range;				// 进度条数值范围
	QPoint m_pos;					// 鼠标点击位置
	PromptBox_rettype m_rettype;	// 记录返回类型
	std::map<QPushButton*, PromptBox_rettype> m_btn_2_btntype;	// <弹框按钮, 弹框返回类型>
	std::map<PromptBox_msgtype, QString> m_msgtype_2_iconpath;	// <弹框消息类型, 对应图标路径>
};
