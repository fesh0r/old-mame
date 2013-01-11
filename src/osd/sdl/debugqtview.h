#ifndef __DEBUG_QT_VIEW_H__
#define __DEBUG_QT_VIEW_H__

#include <QtGui/QtGui>

#include "debug/debugvw.h"


class DebuggerView : public QAbstractScrollArea
{
public:
	DebuggerView(const debug_view_type& type,
					running_machine* machine,
					QWidget* parent=NULL);
	virtual ~DebuggerView() {}

	void paintEvent(QPaintEvent* event);

	// Callback to allow MAME to refresh the view
	static void debuggerViewUpdate(debug_view& debugView, void* osdPrivate);

	// Setters and accessors
	void setPreferBottom(bool pb) { m_preferBottom = pb; }
	debug_view* view() { return m_view; }


protected:
	void keyPressEvent(QKeyEvent* event);


private:
	bool m_preferBottom;

	debug_view* m_view;
	running_machine* m_machine;
};


#endif
