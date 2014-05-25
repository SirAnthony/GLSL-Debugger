#include "watchview.h"
#include "shmappingwidget.h"
#include "shwindowmanager.h"
#include <QAbstractItemModel>

WatchView::WatchView(QWidget *parent) :
	QWidget(parent)
{
	setAttribute(Qt::WA_DeleteOnClose, true);
	type = ShWindowManager::wtNone;
}

void WatchView::closeView()
{
	hide();
	deleteLater();
}

void WatchView::updateGUI()
{
	QString title("");
	QAbstractItemModel *m = model();
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
	connect(widget, SIGNAL(updateView(bool)), this, SLOT(updateView(bool)));
	connect(widget, SIGNAL(updateData(int,int,float,float)),
			this, SLOT(updateData(int,int,float,float)));
	connect(widget, SIGNAL(clearData()), this, SLOT(clearData()));
	connect(widget, SIGNAL(getBoundaries(int,double*,double*,bool)),
			this, SLOT(setBoundaries(int,double*,double*,bool)));
	connect(widget, SIGNAL(getDataBox(int, DataBox**)),
			this, SLOT(setDataBox(int, DataBox**)));
}

void WatchView::clearDataArray(float *data, int count, int dataStride, float clearValue)
{
	for (int i = 0; i < count; i++) {
		*data = clearValue;
		data += dataStride;
	}
}
