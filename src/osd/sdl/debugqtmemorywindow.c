#include "debugqtmemorywindow.h"

#include "debug/dvmemory.h"
#include "debug/debugcon.h"
#include "debug/debugcpu.h"


MemoryWindow::MemoryWindow(running_machine* machine, QWidget* parent) :
	WindowQt(machine, NULL)
{
	setWindowTitle("Debug: Memory View");

	if (parent != NULL)
	{
		QPoint parentPos = parent->pos();
		setGeometry(parentPos.x()+100, parentPos.y()+100, 800, 400);
	}

	//
	// The main frame and its input and log widgets
	//
	QFrame* mainWindowFrame = new QFrame(this);

	// The top frame & groupbox that contains the input widgets
	QFrame* topSubFrame = new QFrame(mainWindowFrame);

	// The input edit
	m_inputEdit = new QLineEdit(topSubFrame);
	connect(m_inputEdit, SIGNAL(returnPressed()), this, SLOT(expressionSubmitted()));

	// The memory space combo box
	m_memoryComboBox = new QComboBox(topSubFrame);
	m_memoryComboBox->setMinimumWidth(300);
	connect(m_memoryComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(memoryRegionChanged(int)));

	// The main memory window
	m_memTable = new DebuggerView(DVT_MEMORY,
									m_machine,
									this);

	// Layout
	QHBoxLayout* subLayout = new QHBoxLayout(topSubFrame);
	subLayout->addWidget(m_inputEdit);
	subLayout->addWidget(m_memoryComboBox);
	subLayout->setSpacing(3);
	subLayout->setContentsMargins(2,2,2,2);

	QVBoxLayout* vLayout = new QVBoxLayout(mainWindowFrame);
	vLayout->setSpacing(3);
	vLayout->setContentsMargins(2,2,2,2);
	vLayout->addWidget(topSubFrame);
	vLayout->addWidget(m_memTable);

	setCentralWidget(mainWindowFrame);

	//
	// Menu bars
	//
	// Create a byte-chunk group
	QActionGroup* chunkGroup = new QActionGroup(this);
	QAction* chunkActOne  = new QAction("1-byte chunks", this);
	chunkActOne->setObjectName("chunkActOne");
	QAction* chunkActTwo  = new QAction("2-byte chunks", this);
	chunkActTwo->setObjectName("chunkActTwo");
	QAction* chunkActFour = new QAction("4-byte chunks", this);
	chunkActFour->setObjectName("chunkActFour");
	chunkActOne->setCheckable(true);
	chunkActTwo->setCheckable(true);
	chunkActFour->setCheckable(true);
	chunkActOne->setActionGroup(chunkGroup);
	chunkActTwo->setActionGroup(chunkGroup);
	chunkActFour->setActionGroup(chunkGroup);
	chunkActOne->setShortcut(QKeySequence("Ctrl+1"));
	chunkActTwo->setShortcut(QKeySequence("Ctrl+2"));
	chunkActFour->setShortcut(QKeySequence("Ctrl+4"));
	chunkActOne->setChecked(true);
	connect(chunkGroup, SIGNAL(triggered(QAction*)), this, SLOT(chunkChanged(QAction*)));

	// Create a address display group
	QActionGroup* addressGroup = new QActionGroup(this);
	QAction* addressActLogical = new QAction("Logical Addresses", this);
	QAction* addressActPhysical = new QAction("Physical Addresses", this);
	addressActLogical->setCheckable(true);
	addressActPhysical->setCheckable(true);
	addressActLogical->setActionGroup(addressGroup);
	addressActPhysical->setActionGroup(addressGroup);
	addressActLogical->setShortcut(QKeySequence("Ctrl+G"));
	addressActPhysical->setShortcut(QKeySequence("Ctrl+Y"));
	addressActLogical->setChecked(true);
	connect(addressGroup, SIGNAL(triggered(QAction*)), this, SLOT(addressChanged(QAction*)));

	// Create a reverse view radio
	QAction* reverseAct = new QAction("Reverse View", this);
	reverseAct->setCheckable(true);
	reverseAct->setShortcut(QKeySequence("Ctrl+R"));
	connect(reverseAct, SIGNAL(toggled(bool)), this, SLOT(reverseChanged(bool)));

	// Create increase and decrease bytes-per-line actions
	QAction* increaseBplAct = new QAction("Increase Bytes Per Line", this);
	QAction* decreaseBplAct = new QAction("Decrease Bytes Per Line", this);
	increaseBplAct->setShortcut(QKeySequence("Ctrl+P"));
	decreaseBplAct->setShortcut(QKeySequence("Ctrl+O"));
	connect(increaseBplAct, SIGNAL(triggered(bool)), this, SLOT(increaseBytesPerLine(bool)));
	connect(decreaseBplAct, SIGNAL(triggered(bool)), this, SLOT(decreaseBytesPerLine(bool)));

	// Assemble the options menu
	QMenu* optionsMenu = menuBar()->addMenu("&Options");
	optionsMenu->addActions(chunkGroup->actions());
	optionsMenu->addSeparator();
	optionsMenu->addActions(addressGroup->actions());
	optionsMenu->addSeparator();
	optionsMenu->addAction(reverseAct);
	optionsMenu->addSeparator();
	optionsMenu->addAction(increaseBplAct);
	optionsMenu->addAction(decreaseBplAct);


	//
	// Initialize
	//
	populateComboBox();

	// Set to the current CPU's memory view
	setToCurrentCpu();
}


void MemoryWindow::memoryRegionChanged(int index)
{
	m_memTable->view()->set_source(*m_memTable->view()->source_list().by_index(index));
	m_memTable->viewport()->update();

	// Update the chunk size radio buttons to the memory region's default
	debug_view_memory* memView = downcast<debug_view_memory*>(m_memTable->view());
	switch(memView->bytes_per_chunk())
	{
		case 1: chunkSizeMenuItem("chunkActOne")->setChecked(true); break;
		case 2: chunkSizeMenuItem("chunkActTwo")->setChecked(true); break;
		case 4: chunkSizeMenuItem("chunkActFour")->setChecked(true); break;
		default: break;
	}
}


void MemoryWindow::expressionSubmitted()
{
	const QString expression = m_inputEdit->text();
	downcast<debug_view_memory*>(m_memTable->view())->set_expression(expression.toLocal8Bit().data());

	// Make the cursor pop
	m_memTable->view()->set_cursor_visible(true);

	// Check where the cursor is and adjust the scroll accordingly
	debug_view_xy cursorPosition = m_memTable->view()->cursor_position();
	// TODO: check if the region is already visible?
	m_memTable->verticalScrollBar()->setValue(cursorPosition.y);

	m_memTable->update();
	m_memTable->viewport()->update();
}


void MemoryWindow::chunkChanged(QAction* changedTo)
{
	debug_view_memory* memView = downcast<debug_view_memory*>(m_memTable->view());
	if (changedTo->text() == "1-byte chunks")
	{
		memView->set_bytes_per_chunk(1);
	}
	else if (changedTo->text() == "2-byte chunks")
	{
		memView->set_bytes_per_chunk(2);
	}
	else if (changedTo->text() == "4-byte chunks")
	{
		memView->set_bytes_per_chunk(4);
	}
	m_memTable->viewport()->update();
}


void MemoryWindow::addressChanged(QAction* changedTo)
{
	debug_view_memory* memView = downcast<debug_view_memory*>(m_memTable->view());
	if (changedTo->text() == "Logical Addresses")
	{
		memView->set_physical(false);
	}
	else if (changedTo->text() == "Physical Addresses")
	{
		memView->set_physical(true);
	}
	m_memTable->viewport()->update();
}


void MemoryWindow::reverseChanged(bool changedTo)
{
	debug_view_memory* memView = downcast<debug_view_memory*>(m_memTable->view());
	memView->set_reverse(changedTo);
	m_memTable->viewport()->update();
}


void MemoryWindow::increaseBytesPerLine(bool changedTo)
{
	debug_view_memory* memView = downcast<debug_view_memory*>(m_memTable->view());
	memView->set_chunks_per_row(memView->chunks_per_row() + 1);
	m_memTable->viewport()->update();
}


void MemoryWindow::decreaseBytesPerLine(bool checked)
{
	debug_view_memory* memView = downcast<debug_view_memory*>(m_memTable->view());
	memView->set_chunks_per_row(memView->chunks_per_row() - 1);
	m_memTable->viewport()->update();
}


void MemoryWindow::populateComboBox()
{
	if (m_memTable == NULL)
		return;

	m_memoryComboBox->clear();
	for (const debug_view_source* source = m_memTable->view()->source_list().head();
			source != NULL;
			source = source->next())
	{
		m_memoryComboBox->addItem(source->name());
	}
}


void MemoryWindow::setToCurrentCpu()
{
	device_t* curCpu = debug_cpu_get_visible_cpu(*m_machine);
	const debug_view_source *source = m_memTable->view()->source_list().match_device(curCpu);
	const int listIndex = m_memTable->view()->source_list().index(*source);
	m_memoryComboBox->setCurrentIndex(listIndex);
}


// I have a hard time storing QActions as class members.  This is a substitute.
QAction* MemoryWindow::chunkSizeMenuItem(const QString& itemName)
{
	QList<QMenu*> menus = menuBar()->findChildren<QMenu*>();
	for (int i = 0; i < menus.length(); i++)
	{
		if (menus[i]->title() != "&Options") continue;
		QList<QAction*> actions = menus[i]->actions();
		for (int j = 0; j < actions.length(); j++)
		{
			if (actions[j]->objectName() == itemName)
				return actions[j];
		}
	}
	return NULL;
}
