#ifndef __DEBUG_QT_BREAK_POINTS_WINDOW_H__
#define __DEBUG_QT_BREAK_POINTS_WINDOW_H__

#include <QtGui/QtGui>

#include "debugqtview.h"
#include "debugqtwindow.h"


//============================================================
//  The Breakpoints Window.
//============================================================
class BreakpointsWindow : public WindowQt
{
	Q_OBJECT

public:
	BreakpointsWindow(running_machine* machine, QWidget* parent=NULL);
	virtual ~BreakpointsWindow();


private:
	// Widgets
	DebuggerView* m_breakpointsView;
};


//=========================================================================
//  A way to store the configuration of a window long enough to read/write.
//=========================================================================
class BreakpointsWindowQtConfig : public WindowQtConfig
{
public:
	BreakpointsWindowQtConfig() :
		WindowQtConfig(WIN_TYPE_BREAK_POINTS)
	{
	}

	~BreakpointsWindowQtConfig() {}

	void buildFromQWidget(QWidget* widget);
	void applyToQWidget(QWidget* widget);
	void addToXmlDataNode(xml_data_node* node) const;
	void recoverFromXmlNode(xml_data_node* node);
};


#endif
