#ifndef SHWINDOWMANAGER_H
#define SHWINDOWMANAGER_H

#include <QObject>
#include <QWorkspace>
#include "models/shvaritem.h"

class ShWindowManager : public QWorkspace
{
	Q_OBJECT
public:
	explicit ShWindowManager(QWidget *parent = 0);

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
	void createWindow(const QList<ShVarItem*>&, enum WindowType);

private:

};

#endif // SHWINDOWMANAGER_H
