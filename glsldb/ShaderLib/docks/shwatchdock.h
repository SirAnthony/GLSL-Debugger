
#ifndef SHWATCHDOCK_H
#define SHWATCHDOCK_H

#include "shdockwidget.h"
#include "watch/shwindowmanager.h"
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

	void expand(ShVarItem *item);

signals:
	void createWindow(const QList<ShVarItem*>&, int);
	void extendWindow(const QList<ShVarItem*>&, int);

public slots:
	void updateGui(bool enable);
	virtual void cleanDock(ShaderMode type);
	void updateData(CoverageMapStatus cmstatus, ShaderMode type, bool force);
	void resetData(ShaderMode type);
	void updateCoverage(ShaderMode type, bool *coverage);
	void selectionChanged();
	void newItem(ShVarItem* item);	
	void removeSelected();
	void clearWatchList();
	void updateSelection(int, int, QString &, ShaderMode);

	void newWindow();
	void extendWindow();

protected:	
	int getWindowType();
	int getItems(QList<ShVarItem *> &);
	QModelIndexList filterSelection(const QModelIndexList& input);

private:
	QSet<ShVarItem *> watchItems;
	Ui::ShWatchDock *ui;
};


#endif // SHWATCHDOCK_H
