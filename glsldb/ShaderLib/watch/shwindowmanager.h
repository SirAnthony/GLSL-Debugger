#ifndef SHWINDOWMANAGER_H
#define SHWINDOWMANAGER_H

#include <QObject>
#include <QWorkspace>
#include "models/shvaritem.h"
class QAction;
class WatchView;

struct MenuActions {
	QAction *zoom;
	QAction *selectPixel;
	QAction *minMaxLens;
};

class ShWindowManager : public QWorkspace
{
	Q_OBJECT
public:
	explicit ShWindowManager(QWidget *parent = 0);
	void addActions(QWidget *);

	enum WindowType {
		wtNone,
		wtVertex,
		wtGeometry,
		wtFragment
	};

signals:

public slots:
	void updateWindows(bool);
	void changedActive(QWidget*);	
	void createWindow(const QList<ShVarItem*>&, int);
	void extendWindow(const QList<ShVarItem*>&, int);
	void windowClosed();

protected:
	void attachData(WatchView *, const QList<ShVarItem*> &, enum WindowType);

private:
	struct MenuActions menuActions;

};

#endif // SHWINDOWMANAGER_H
