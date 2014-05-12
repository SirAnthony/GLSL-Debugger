#include "watchview.h"
#include "shmappingwidget.h"
#include <QAbstractItemModel>

WatchView::WatchView(QWidget *parent) :
	QWidget(parent)
{
	setAttribute(Qt::WA_DeleteOnClose, true);
}

void WatchView::updateGUI()
{
	QString title("");
	QAbstractItemModel *m = _model();
	int columns = m->columnCount();
	for (int i = 0; i < columns; i++) {
		if (i > 0)
			title += ", ";
		title += m->headerData(i, Qt::Horizontal).toString();
	}
	setWindowTitle(title);
}

void WatchView::connectWidget(ShMappingWidget *widget)
{
	connect(widget, SIGNAL(updateData(int,int,float,float)),
			this, SLOT(updateData(int,int,float,float)));

}
