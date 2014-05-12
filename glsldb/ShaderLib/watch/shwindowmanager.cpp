#include "shwindowmanager.h"
#include "watchview.h"
#include "watchtable.h"
#include "utils/dbgprint.h"

ShWindowManager::ShWindowManager(QObject *parent) :
	QObject(parent)
{
	connect(this, SIGNAL(windowActivated(QWidget*)), this, SLOT(changedActive(QWidget*)));

}

void ShWindowManager::updateWindows(bool)
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


void ShWindowManager::createWindow(const QList<ShVarItem*>& list, enum WindowType type)
{
	if (type == wtNone) {
		dbgPrint(DBGLVL_ERROR, "Wrong window type");
		return;
	}

	WatchView *window = NULL;
	foreach(ShVarItem* item, list) {
		if (!window) {
			if (type == wtVertex)
				window = new WatchTable(this);
			else if (type == wtGeometry)
				window = new WatchGeoTree(m_primitiveMode,
										  m_dShResources.geoOutputType, m_pGeometryMap,
										  m_pVertexCount, this);
			else if (type == wtFragment)
				window = new WatchVector(this);

			connect(window, SIGNAL(selectionChanged(int)), this, SLOT(newSelectedVertex(int)));
			connect(window, SIGNAL(selectionChanged(int)), this, SLOT(newSelectedPrimitive(int)));
			connect(window, SIGNAL(selectionChanged(int, int)), this, SLOT(newSelectedPixel(int, int)));

			if (type == wtFragment) {
				connect(window, SIGNAL(destroyed()), this, SLOT(watchWindowClosed()));
				connect(window, SIGNAL(mouseOverValuesChanged(int, int, const bool *, const QVariant *)),
						this, SLOT(setMouseOverValues(int, int, const bool *, const QVariant *)));
				connect(window, SIGNAL(selectionChanged(int, int)),
						this, SLOT(newSelectedPixel(int, int)));
				connect(aZoom, SIGNAL(triggered()), window, SLOT(setZoomMode()));
				connect(aSelectPixel, SIGNAL(triggered()), window, SLOT(setPickMode()));
				connect(aMinMaxLens, SIGNAL(triggered()), window, SLOT(setMinMaxMode()));
				/* initialize mouse mode */
				agWatchControl->setEnabled(true);
				if (agWatchControl->checkedAction() == aZoom) {
					window->setZoomMode();
				} else if (agWatchControl->checkedAction() == aSelectPixel) {
					window->setPickMode();
				} else if (agWatchControl->checkedAction() == aMinMaxLens) {
					window->setMinMaxMode();
				}

				window->setWorkspace(this);
				this->addWindow(window);
			}
		}
		window->attachData(item);
	}
}
