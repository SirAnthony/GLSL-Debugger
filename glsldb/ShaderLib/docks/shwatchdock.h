
#ifndef SHWATCHDOCK_H
#define SHWATCHDOCK_H

#include "shdockwidget.h"
#include "watch/shwindowmanager.h"
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

	virtual void registerDock(ShDataManager *);
	void expand(ShVarItem *item);

signals:
	void createWindow(const QList<ShVarItem*>&, int);
	void extendWindow(const QList<ShVarItem*>&, int);

public slots:
	void updateGui(bool);
	virtual void cleanDock(ShaderMode);
	void updateData(ShaderMode, CoverageMapStatus, bool *, bool);
	void resetData(ShaderMode, bool *);
	void updateCoverage(ShaderMode, bool *);
	void selectionChanged();
	void newItem(ShVarItem *);
	void removeSelected();
	void clearWatchList();
	void updateSelection(int, int, QString &, ShaderMode);
	void getWatchItems(QSet<ShVarItem *> &);

	void newWindow();
	void extendWindow();

protected:
	int getWindowType();
	int getItems(QList<ShVarItem *> &);
	QModelIndexList filterSelection(const QModelIndexList &);

private:
	QSet<ShVarItem *> watchItems;
	Ui::ShWatchDock *ui;
};


#endif // SHWATCHDOCK_H
