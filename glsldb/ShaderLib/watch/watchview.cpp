#include "watchview.h"

WatchView::WatchView(QWidget *parent) :
	QWidget(parent)
{
	setAttribute(Qt::WA_DeleteOnClose, true);
}
