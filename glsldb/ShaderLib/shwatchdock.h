#ifndef SHWATCHDOCK_H
#define SHWATCHDOCK_H

#include "shdockwidget.h"
#include <QSet>

namespace Ui {
	class ShWatchDock;
}

class ShWatchDock : public ShDockWidget
{
	Q_OBJECT
public:
	explicit ShWatchDock(QWidget *parent = 0);
	~ShWatchDock();

	void expand(ShVarItem* item);

public slots:
	void newItem(ShVarItem* item);
	void removeItems();

private:
	QSet<ShVarItem *> watchItems;
	Ui::ShWatchDock *ui;
};


#endif // SHWATCHDOCK_H
