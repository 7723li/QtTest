#include "PromptBox.h"

PromptBox_kit::PromptBox_kit(PromptBox* p)
{
	QFont tip_font;
	tip_font.setPixelSize(16);	// 详见《V100 Pro软件需求说明书》节5.1.3
	tip_font.setFamily("Microsoft YaHei");

	QFont big_tip_font;
	big_tip_font.setPixelSize(19);	// 详见《V100 Pro软件需求说明书》节5.1.3
	big_tip_font.setFamily("Microsoft YaHei");

	background_main = new QWidget(p);
	background_main->setObjectName("widgetGrayContainer");
	background_main->setGeometry(0, 0, p->width(), p->height());

	background_tips_progbar = new QWidget(background_main);
	background_tips_progbar->setGeometry(30, 30, 437, 166);

	backgound_cancle_confirm = new QWidget(background_main);
	backgound_cancle_confirm->setObjectName("widgetBottomShelter");
	backgound_cancle_confirm->setGeometry(0, 190, p->width(), 36);

	close_btn = new QPushButton(background_main);
	close_btn->setGeometry(468, 0, 29, 29);
	close_btn->setObjectName("pushButtonCloseMiddle");

	cancle_btn = new QPushButton(backgound_cancle_confirm);
	cancle_btn->setGeometry(300, 3, 85, 29);
	cancle_btn->setText(QStringLiteral("取消"));
	cancle_btn->setObjectName("pushButtonOperation");

	confirm_btn = new QPushButton(backgound_cancle_confirm);
	confirm_btn->setGeometry(400, 3, 85, 29);
	confirm_btn->setText(QStringLiteral("确认"));
	confirm_btn->setObjectName("pushButtonOperation");

	m_using_kitlist = new QStackedWidget(background_tips_progbar);
	m_using_kitlist->setGeometry(0, 0, background_tips_progbar->width(), background_tips_progbar->height());

	// msgbox_kitlist
	; {
		msgbox_kitlist = new struct _msgbox_kitlist;

		msgbox_kitlist->tips = new QLabel(msgbox_kitlist);
		msgbox_kitlist->tips->setGeometry(100, 63, 300, 40);
		msgbox_kitlist->tips->setFont(tip_font);

		msgbox_kitlist->_icon = new QLabel(msgbox_kitlist);
		msgbox_kitlist->_icon->setGeometry(40, 58, 50, 50);
	}

	// progbar_kitlist
	; {
		progbar_kitlist = new struct _progbar_kitlist;

		progbar_kitlist->progbar = new QProgressBar(progbar_kitlist);
		progbar_kitlist->progbar->setTextVisible(false);
		progbar_kitlist->progbar->setObjectName("QProgressBar");
		progbar_kitlist->progbar->setGeometry(19, 80, 399, 30);

		progbar_kitlist->progbar_schedule = new QLabel(progbar_kitlist);
		progbar_kitlist->progbar_schedule->setGeometry(378, 120, 40, 40);

		progbar_kitlist->tips = new QLabel(progbar_kitlist);
		progbar_kitlist->tips->setFont(tip_font);
		progbar_kitlist->tips->setGeometry(19, 40, 200, 30);
	}

	// batchprogbar_kitlist
	; {
		batchprogbar_kitlist = new struct _batchprogbar_kitlist;

		batchprogbar_kitlist->progbar = new QProgressBar(batchprogbar_kitlist);
		batchprogbar_kitlist->progbar->setGeometry(19, 55, 399, 60);
		batchprogbar_kitlist->progbar->setTextVisible(false);

		batchprogbar_kitlist->tip_exporting = new QLabel(batchprogbar_kitlist);
		batchprogbar_kitlist->tip_exporting->setGeometry(19, 5, 399, 20);
		batchprogbar_kitlist->tip_exporting->setFont(tip_font);

		batchprogbar_kitlist->tip_exportnum = new QLabel(batchprogbar_kitlist);
		batchprogbar_kitlist->tip_exportnum->setGeometry(19, 28, 399, 20);
		batchprogbar_kitlist->tip_exportnum->setFont(big_tip_font);

		batchprogbar_kitlist->tip_finished = new QLabel(batchprogbar_kitlist);
		batchprogbar_kitlist->tip_finished->setGeometry(19, 110, 399, 20);
		batchprogbar_kitlist->tip_finished->setFont(tip_font);

		batchprogbar_kitlist->tip_residuetime = new QLabel(batchprogbar_kitlist);
		batchprogbar_kitlist->tip_residuetime->setGeometry(19, 133, 399, 20);
		batchprogbar_kitlist->tip_residuetime->setFont(tip_font);
	}

	m_using_kitlist->addWidget(msgbox_kitlist);
	m_using_kitlist->addWidget(progbar_kitlist);
	m_using_kitlist->addWidget(batchprogbar_kitlist);
	m_using_kitlist->setCurrentWidget(msgbox_kitlist);
}

PromptBox* PromptBox::s_inst = nullptr;
PromptBox::PromptBox()
{
	this->setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
	this->setWindowModality(Qt::ApplicationModal);
	this->setGeometry(500, 500, 497, 226);
	this->setObjectName("widgetGrayContainer");

	m_loop = new QEventLoop(this);

	m_msgbox_timer = new QTimer(this);
	m_msgbox_timer->setSingleShot(true);
	connect(m_msgbox_timer, &QTimer::timeout, m_loop, &QEventLoop::quit);

	m_progbar_timer = new QTimer(this);
	connect(m_progbar_timer, &QTimer::timeout, this, &PromptBox::on_progtimer_timeout);

	m_kit = new PromptBox_kit(this);
	m_using_bar = m_kit->progbar_kitlist->progbar;	// 默认值 反正不让为空指针就ok

	connect(m_kit->close_btn, &QPushButton::clicked, this, &PromptBox::on_kitbtn_clicked);
	connect(m_kit->cancle_btn, &QPushButton::clicked, this, &PromptBox::on_kitbtn_clicked);
	connect(m_kit->confirm_btn, &QPushButton::clicked, this, &PromptBox::on_kitbtn_clicked);

	m_prog_autoclose = false;
	m_prog_range = 0;
	m_rettype = PromptBox_rettype::Error;
	m_btn_2_btntype[m_kit->close_btn] = PromptBox_rettype::Closed;
	m_btn_2_btntype[m_kit->cancle_btn] = PromptBox_rettype::Cancled;
	m_btn_2_btntype[m_kit->confirm_btn] = PromptBox_rettype::Confirmed;

	m_msgtype_2_iconpath[PromptBox_msgtype::Information] = "./Resources/icon/Information.png";
	m_msgtype_2_iconpath[PromptBox_msgtype::Warning] = "./Resources/icon/Warning.png";
	m_msgtype_2_iconpath[PromptBox_msgtype::Question] = "./Resources/icon/Question.png";
	m_msgtype_2_iconpath[PromptBox_msgtype::Succeed] = "./Resources/icon/True.png";
	m_msgtype_2_iconpath[PromptBox_msgtype::Failed] = "./Resources/icon/False.png";
}
PromptBox::~PromptBox()
{

}

PromptBox* PromptBox::inst(QWidget* p)
{
	if (nullptr == s_inst)
		s_inst = new PromptBox;

	s_inst->setParent(p);
	return s_inst;
}

PromptBox_rettype PromptBox::msgbox_go(PromptBox_msgtype msgtype, PromptBox_btntype btntype, tiptype show_tips, int interval_ms, bool auto_close)
{
	set_MsgBox_style(msgtype, btntype, show_tips);
	if (auto_close)
	{
		m_kit->close_btn->hide();
	}
	else
	{
		m_kit->close_btn->show();
	}
	this->show();

	if (auto_close)
	{
		m_msgbox_timer->start(interval_ms);
		m_rettype = PromptBox_rettype::Closed;			// 这种情况默认返回关闭状态
	}

	m_loop->exec();

	stop(auto_close);

	return m_rettype;
}

void PromptBox::progbar_prepare(int min, int max, Prompt_progbar_type bartype, bool auto_close)
{
	// 进度条的主要步骤 其实也就启动个定时器
	switch (bartype)
	{
	case Prompt_progbar_type::Normal_Progbar:
		set_Progress_style();
		break;
	case Prompt_progbar_type::Batch_Progbar:
		set_BatchProg_style();
		break;
	default:
		set_Progress_style();
		break;
	}

	if (auto_close)
	{
		m_kit->close_btn->hide();
	}
	else
	{
		m_kit->close_btn->show();
	}

	m_prog_autoclose = auto_close;
	m_prog_range = max;

	m_kit->progbar_kitlist->progbar_schedule->setText(QString("%1%").arg(0));
	m_kit->progbar_kitlist->progbar->setMinimum(min);
	m_kit->progbar_kitlist->progbar->setMaximum(max);

	this->show();
	m_progbar_timer->start(33);
}
void PromptBox::progbar_go(int val, tiptype tips)
{
	// 只是纯粹设置数值 判断在定时器槽函数中做
	if (!m_progbar_timer->isActive())
		return;

	m_kit->progbar_kitlist->progbar->setValue(val);
	m_kit->progbar_kitlist->tips->setText(tips);
	m_kit->progbar_kitlist->progbar_schedule->setText(QString("%1%").arg(val * 100 / m_prog_range));

	QApplication::processEvents();
}
void PromptBox::progbar_go(int val, tiptype tip_exporting, tiptype tip_exportnum, tiptype tip_finished, tiptype tip_residuetime)
{
	if (!m_progbar_timer->isActive())
		return;

	m_kit->batchprogbar_kitlist->progbar->setValue(val);
	m_kit->batchprogbar_kitlist->tip_exporting->setText(tip_exporting);
	m_kit->batchprogbar_kitlist->tip_exportnum->setText(tip_exportnum);
	m_kit->batchprogbar_kitlist->tip_finished->setText(tip_finished);
	m_kit->batchprogbar_kitlist->tip_residuetime->setText(tip_residuetime);

	QApplication::processEvents();
}

void PromptBox::progbar_fakego(int min, int max, int interval_ms, QString tip, bool auto_close)
{
	progbar_prepare(min, max, Prompt_progbar_type::Normal_Progbar, auto_close);
	for (int i = 0; i <= max; ++i)
	{
		::Sleep(interval_ms);
		progbar_go(i, tip);
	}
}

void PromptBox::mousePressEvent(QMouseEvent* e)
{
	m_pos = e->globalPos() - this->pos();
	e->accept();
}
void PromptBox::mouseMoveEvent(QMouseEvent* e)
{
	this->move(e->globalPos() - m_pos);
	e->accept();
}
void PromptBox::mouseReleaseEvent(QMouseEvent* e)
{

}

void PromptBox::closeEvent(QCloseEvent* e)
{
	stop(true);
}

void PromptBox::set_MsgBox_style(PromptBox_msgtype msgtype, PromptBox_btntype btntype, QString show_tip)
{
	m_kit->close_btn->show();
	switch (btntype)
	{
	case PromptBox_btntype::None:
		m_kit->cancle_btn->hide();
		m_kit->confirm_btn->hide();
		break;
	case PromptBox_btntype::Confirm:
		m_kit->cancle_btn->hide();
		m_kit->confirm_btn->show();
		break;
	case PromptBox_btntype::Cancle:
		m_kit->cancle_btn->show();
		m_kit->confirm_btn->hide();
		break;
	case PromptBox_btntype::Confirm_and_Cancle:
		m_kit->cancle_btn->show();
		m_kit->confirm_btn->show();
		break;
	default:
		break;
	}

	m_kit->m_using_kitlist->setCurrentWidget(m_kit->msgbox_kitlist);

	m_kit->msgbox_kitlist->tips->setText(show_tip);

	auto iconpath_iter = m_msgtype_2_iconpath.find(msgtype);
	if (iconpath_iter == m_msgtype_2_iconpath.end())
	{
		msgtype = PromptBox_msgtype::Information;
		iconpath_iter = m_msgtype_2_iconpath.find(msgtype);
	}
	QString iconpath = iconpath_iter->second;
	m_kit->msgbox_kitlist->_icon->setPixmap(QPixmap(iconpath).scaled(m_kit->msgbox_kitlist->_icon->size()));
}
void PromptBox::set_Progress_style()
{
	m_kit->close_btn->show();
	m_kit->cancle_btn->show();
	m_kit->confirm_btn->hide();

	m_kit->m_using_kitlist->setCurrentWidget(m_kit->progbar_kitlist);

	m_using_bar = m_kit->progbar_kitlist->progbar;
}
void PromptBox::set_BatchProg_style()
{
	m_kit->close_btn->show();
	m_kit->cancle_btn->show();
	m_kit->confirm_btn->hide();

	m_kit->m_using_kitlist->setCurrentWidget(m_kit->batchprogbar_kitlist);

	m_using_bar = m_kit->batchprogbar_kitlist->progbar;
}

void PromptBox::stop(bool close_)
{
	// 关掉一系列东西
	if (m_msgbox_timer->isActive())
		m_msgbox_timer->stop();

	if (m_progbar_timer->isActive())
	{
		m_progbar_timer->stop();
		emit Prompt_probar_curval(m_using_bar->value());
		emit Prompt_probar_finish();
	}

	if (m_loop->isRunning())
		m_loop->quit();

	m_prog_range = 0;

	if (close_)
	{
		m_using_bar->reset();
		this->hide();
	}
}

void PromptBox::on_kitbtn_clicked()
{
	QObject* sig_src = this->sender();
	QPushButton* btn = static_cast<QPushButton*>(sig_src);

	auto msgtype_iter = m_btn_2_btntype.find(btn);
	m_rettype = (msgtype_iter != m_btn_2_btntype.end()) ? msgtype_iter->second : PromptBox_rettype::Error;

	return stop(true);
}

void PromptBox::on_progtimer_timeout()
{
	int bar_val = m_using_bar->value(), bar_max = m_using_bar->maximum();
	if (bar_val < bar_max)
	{
		emit Prompt_probar_curval(bar_val);	// 发送当前位置 人人为我 我为人人
	}
	if (bar_val == bar_max)
	{
		m_progbar_timer->stop();			// 预防手动取消时进入暴走状态
		if (m_prog_autoclose)
			stop(m_prog_autoclose);			// 进度条的核心函数 到位了就关掉进度条
		emit Prompt_probar_curval(bar_val);
		emit Prompt_probar_finish();
	}
	else if (bar_val > bar_max)				// 不正常状态 建议吃点肾宝
	{
		stop(true);
	}
}
