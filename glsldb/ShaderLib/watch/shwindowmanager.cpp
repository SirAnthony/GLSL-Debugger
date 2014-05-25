#include "shwindowmanager.h"
#include "watchview.h"
#include "watchtable.h"
#include "watchgeotree.h"
#include "watchvector.h"
#include "shdatamanager.h"
#include "utils/dbgprint.h"
#include <QAction>
#include <QMenu>

ShWindowManager::ShWindowManager(QWidget *parent) :
	QWorkspace(parent)
{
	connect(this, SIGNAL(windowActivated(QWidget*)), this, SLOT(changedActive(QWidget*)));

	menuActions.zoom = new QAction(QIcon(":/icons/icons/system-search_32.png"),
							   "&Zoom Mode", this);
	menuActions.zoom->setStatusTip(tr("Zoom Mode"));
	menuActions.selectPixel = new QAction(QIcon(":/icons/icons/crosshair_32.png"),
									  "&Select Pixel Mode", this);
	menuActions.selectPixel->setStatusTip(tr("Select Pixel Mode"));
	menuActions.minMaxLens = new QAction(QIcon(":/icons/icons/min-max-lens.png"),
										 "&Min Max Lens", this);
	menuActions.minMaxLens->setStatusTip(tr("Min Max Lens"));
}

void ShWindowManager::addActions(QWidget *menu)
{
	menu->addAction(menuActions.zoom);
	menu->addAction(menuActions.selectPixel);
	menu->addAction(menuActions.minMaxLens);
}

void ShWindowManager::updateWindows(bool force)
{
	foreach (QWidget* widget, this->windowList()) {
		WatchView* view = dynamic_cast<WatchView*>(widget);
		view->updateView(force);
	}
}

void ShWindowManager::changedActive(QWidget *active)
{
	foreach (QWidget* widget, this->windowList())
		dynamic_cast<WatchView*>(widget)->setActive(widget == active);
}

void ShWindowManager::createWindow(const QList<ShVarItem*> &list, enum WindowType type)
{
	if (type == wtNone) {
		dbgPrint(DBGLVL_ERROR, "Wrong window type");
		return;
	}

	ShDataManager *manager = ShDataManager::get();
	GeometryInfo geometry = manager->getGeometryInfo();

	WatchView *window;
	if (type == wtVertex) {
		window = new WatchTable(this);
	} else if (type == wtGeometry) {
		window = new WatchGeoTree(&geometry, this);
	} else if (type == wtFragment) {
		WatchVector* vector = new WatchVector(this);
		window = vector;
		vector->connectActions(&menuActions);
		connect(window, SIGNAL(destroyed()), this, SLOT(windowClosed()));
		connect(vector, SIGNAL(mouseOverValuesChanged(int,int,const bool*,const QVariant*)),
				this, SLOT(setMouseOverValues(int, int, const bool *, const QVariant *)));
	}

	this->addWindow(window);
	connect(window, SIGNAL(selectionChanged(int)), this, SLOT(newSelectedVertex(int)));
	connect(window, SIGNAL(selectionChanged(int)), this, SLOT(newSelectedPrimitive(int)));
	connect(window, SIGNAL(selectionChanged(int, int)), this, SLOT(newSelectedPixel(int, int)));
	attachData(window, list, type);
}


void ShWindowManager::extendWindow(const QList<ShVarItem *> &list, enum WindowType type)
{
	WatchView *window = dynamic_cast<WatchView*>(this->activeWindow());
	if (!window)
		return;

	attachData(window, list, type);
}

void ShWindowManager::windowClosed()
{
	int fragments = 0;
	foreach(QWidget *widget, this->windowList()) {
		const WatchView* view = dynamic_cast<WatchView *>(widget);
		if (view->getType() == wtFragment)
			fragments++;
	}

	if (!fragments) {
		menuActions.minMaxLens->setEnabled(false);
		menuActions.selectPixel->setEnabled(false);
		menuActions.zoom->setEnabled(false);
	}
}

void ShWindowManager::attachData(WatchView * window, const QList<ShVarItem *> &list,
								 enum WindowType type)
{
	if (!window || type != window->getType())
		return;

	foreach(ShVarItem* item, list) {
		QString name = item->data(DF_FULLNAME).toString();
		if (type == wtVertex)
			window->attachData(static_cast<DataBox*>(
								   item->data(DF_DATA_VERTEXBOX).value<void*>()), name);
		else if (type == wtFragment)
			window->attachData(static_cast<DataBox*>(
								   item->data(DF_DATA_PIXELBOX).value<void*>()), name);
		else if (type == wtGeometry)
			window->attachData(static_cast<DataBox*>(
								   item->data(DF_DATA_GEOMETRYBOX).value<void*>()),
							   static_cast<DataBox*>(
								   item->data(DF_DATA_VERTEXBOX).value<void*>()), name);
	}

	window->updateView(true);
}
