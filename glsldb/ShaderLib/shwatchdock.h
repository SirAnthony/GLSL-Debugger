#ifndef SHWATCHDOCK_H
#define SHWATCHDOCK_H

#include "shdockwidget.h"
#include <QSet>

namespace Ui {
	class ShWatchDock;
}

enum CoverageMapStatus {
	COVERAGEMAP_UNCHANGED,
	COVERAGEMAP_GROWN,
	COVERAGEMAP_SHRINKED
};

class ShWatchDock : public ShDockWidget
{
	Q_OBJECT
public:
	explicit ShWatchDock(QWidget *parent = 0);
	~ShWatchDock();

	void expand(ShVarItem* item);

signals:
	void updateWindows(bool force);

public slots:
	void updateData(CoverageMapStatus cmstatus, bool force);
	void newItem(ShVarItem* item);
	void removeSelected();
	void clearWatchList();

private:
	QSet<ShVarItem *> watchItems;
	Ui::ShWatchDock *ui;
};


#endif // SHWATCHDOCK_H
