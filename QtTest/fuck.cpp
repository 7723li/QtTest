#include "fuck.h"

Fuck::Fuck(QWidget *parent)
	: QWidget(parent)
{
	ui.setupUi(this);
	this->setGeometry(QApplication::desktop()->screenGeometry());
	this->setWindowFlags(Qt::WindowType::FramelessWindowHint);

	btn_1 = new QPushButton("123", this);
	btn_1->setGeometry(0, 0, 100, 30);
	connect(btn_1, &QPushButton::clicked, this, &Fuck::slot_1);

	btn_2 = new QPushButton("456", this);
	btn_2->setGeometry(110, 0, 100, 30);
	connect(btn_2, &QPushButton::clicked, this, &Fuck::slot_switch_sth_widg);

	btn_3 = new QPushButton("789", this);
	btn_3->setGeometry(220, 0, 100, 30);
	connect(btn_3, &QPushButton::clicked, this, &Fuck::slot_enter_PageVideoCollect);

	timer_useless = new QTimer(this);
	loop_useless = new QEventLoop(this);

	running = true;

	m_using_resolution = this->geometry();
	connect(QApplication::desktop(), &QDesktopWidget::resized, this, &Fuck::slot_resolution_resize);

	m_stackwidget = new QStackedWidget(this);
	m_stackwidget->setGeometry(0, btn_1->y() + btn_1->height() + 2, this->width(), this->height() - (btn_1->y() + btn_1->height() + 2));

	sth_widg = new sth_widget(this);
	m_stackwidget->addWidget(sth_widg);
	sth_widg->setGeometry(m_stackwidget->geometry());

	m_PageVideoCollect = new PageVideoRecord(this);
	m_PageVideoCollect->setGeometry(m_stackwidget->geometry());
	m_stackwidget->addWidget(m_PageVideoCollect);
	connect(m_PageVideoCollect, &PageVideoRecord::PageVideoRecord_exit, this, &Fuck::slot_exit_PageVideoCollect);

	m_stackwidget->setCurrentWidget(sth_widg);

	qDebug() << "---init this->geometry() : " << this->geometry();
	qDebug() << "---init btn->geometry() : " << btn_1->geometry();
	qDebug() << "---init st_w->btn_1->geometry() : " << sth_widg->btn_1->geometry();
	qDebug() << "---init sPromptBoxInst->geometry() : " << PromptBoxInst()->geometry();
	qDebug() << "---init m_PageVideoCollect->geometry() : " << m_PageVideoCollect->geometry();

	QFile qss_file("./Resources/common.qss");
	if (qss_file.open(QIODevice::ReadOnly))
	{
		qApp->setStyleSheet(qss_file.readAll());
	}
}

Fuck::~Fuck()
{
	
}

void Fuck::slot_1()
{
 	PromptBox_rettype a = PromptBoxInst()->msgbox_go(PromptBox_msgtype::Succeed, PromptBox_btntype::Confirm_and_Cancle, QStringLiteral("是否手动终结主线程循环"), false);
 	if (a == PromptBox_rettype::Confirmed)
 	{
 		connect(PromptBox::inst(), &PromptBox::Prompt_probar_finish, this, &Fuck::slot_finish_loop);
 	}

 	running = true;
 	PromptBoxInst()->progbar_prepare(0, 100, Prompt_progbar_type::Normal_Progbar, false);
 	for (int i = 0; running && i <= 100; ++i)
 	{
 		::Sleep(10);		// 模拟主线程阻塞
 		PromptBoxInst()->progbar_go_mainthread_blocking(i, QStringLiteral("随便写些东西上去"));
 	}

 	PromptBoxInst()->progbar_prepare(0, 100, Prompt_progbar_type::Batch_Progbar, false);
 	for (int i = 0; running && i <= 100; ++i)
 	{
 		::Sleep(10);		// 模拟主线程阻塞
 		PromptBoxInst()->progbar_go_mainthread_blocking(i, 
 			QStringLiteral("正在导出\"哈哈\"的病例"), 
 			QStringLiteral("已完成\"80%\""), 
 			QStringLiteral("导出病例6个"), 
 			QStringLiteral("剩余时间一亿八千万年")
 			);
 	}

 	 PromptBoxInst()->progbar_fakego(0, 100, 30, "123", true);
}

void Fuck::slot_finish_loop()
{
	disconnect(PromptBox::inst(), &PromptBox::Prompt_probar_finish, this, &Fuck::slot_finish_loop);
	running = false;
}

void Fuck::slot_resolution_resize(int useless)
{
	/*
	思路：
	0、定义一个控件作为顶端 并遍历其子控件 然后以子为父 重复操作 一直遍历至 ->每 条 分 支<- 的最末端
	获取继承自"QWidget"的子控件listA{wB, wC} 并从listA中继续遍历
	main()
	|
	wA----------------------------------|
	|									|
	wB(parent = wA)-|					wC(parent = wA)-|
	|				|					|				|
	wD(parent = wB)	wE(parent = wB)		wF(parent = wC)	wG(Parent = wC)
	|				|					|				|
	...				...					...				...

	1、文字描述比较苍白 看递归版伪代码 简单暴力
	func begin()
	{
		top = this;					// 顶端
		resize(top);				// 更改大小
		findson(top->children());	// 子控件
	}
	func findson(children)
	{
		for(auto child : children)	// 以子为父
		{
			resize(child);			// 更改大小
			findson(chile->children());
		}
	}
	2、在本项目中 这个顶端很显然必须为在main函数中进行初始化的TopV2MainFrame
	3、递归展开成遍历 减少系统资源消耗 以及 出现崩溃的风险
	*/
	QRect changed_resolution = this->geometry();

	// lzx 20200427 缩放倍数 需要结合特殊定义的分辨率 日后再说
	double zoom_multiples = (double)changed_resolution.height() / (double)m_using_resolution.height();
	qDebug() << "from " << changed_resolution << " to: " << m_using_resolution;
	qDebug() << "zoom_multiples : " << zoom_multiples;
	m_using_resolution = changed_resolution;

	// lzx parent与children应随时保证1:1关系 至于为什么不用map 因为map的插入性能比较屎 而这个数据结构主要操作为插入删除 list性能更高(理论上)
	std::list<QObject*> t_list_parent({ this, PromptBoxInst()  });										// 父控件集合
	std::list<QObjectList> t_list_children({ this->children(), PromptBoxInst()->children()  });		// 子控件列表集合

	// lzx 结构 <控件地址 : 找到过了没> 避免有些控件被重复缩放 主要操作为寻找 第二个变量随便 使用static大概可以避免重复插入()
	static std::map<QWidget*, bool> t_map_found_widget;

	while (!t_list_children.empty())									// 一路遍历到尽头 (找到刚好在QWidget下一层的子类控件类)
	{
		QObject* parent = t_list_parent.front();						// 父控件
		if (nullptr == parent)
			continue;

		const QObjectList& children = t_list_children.front();			// 子控件列表

		if (parent != this && parent->inherits("QWidget"))				// 顶端控件在接收resized信号时将会自动缩放 无需手动 & 是否继承自QWidget直接影响到是否可以转换尺寸
		{
			QWidget* parent_widget = static_cast<QWidget*>(parent);
			if (t_map_found_widget.find(parent_widget) == t_map_found_widget.end())
				t_map_found_widget[parent_widget] = true;
		}

		for (QObject* child : children)									// 遍历子控件列表
		{
			t_list_parent.push_back(child);								// 子承父业
			t_list_children.push_back(child->children());
		}

		t_list_parent.pop_front();
		t_list_children.pop_front();
	}

	for (auto & iter : t_map_found_widget)
	{
		QRect old_rect = iter.first->geometry();
		int change_X = (int)((double)old_rect.x() * zoom_multiples);
		int change_Y = (int)((double)old_rect.y() * zoom_multiples);
		int change_Width = (int)((double)old_rect.width() * zoom_multiples);
		int change_Height = (int)((double)old_rect.height() * zoom_multiples);
		iter.first->setGeometry(change_X, change_Y, change_Width, change_Height);
	}

	qDebug() << "---now this->geometry() : " << this->geometry();
	qDebug() << "---now btn->geometry() : " << btn_1->geometry();
	qDebug() << "---now st_w->btn_1->geometry() : " << sth_widg->btn_1->geometry();
	qDebug() << "---now sPromptBoxInst->geometry() : " << PromptBoxInst()->geometry();
	qDebug() << "---now m_PageVideoCollect->geometry() : " << m_PageVideoCollect->geometry();
}

void Fuck::slot_switch_sth_widg()
{
	m_stackwidget->setCurrentWidget(sth_widg);
}
void Fuck::slot_enter_PageVideoCollect()
{
	m_stackwidget->setCurrentWidget(m_PageVideoCollect);
	m_PageVideoCollect->enter_PageVideoRecord("");
}

void Fuck::slot_exit_PageVideoCollect(const QString & examid)
{
	m_stackwidget->setCurrentWidget(sth_widg);
}
