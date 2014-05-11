
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
	void createWindow(const QList<ShVarItem*>&, ShWindowManager::WindowType);

public slots:
	void updateGui(bool enable);
	virtual void cleanDock(EShLanguage type);
	void updateData(CoverageMapStatus cmstatus, EShLanguage type, bool force);
	void resetData(EShLanguage type);
	void updateCoverage(EShLanguage type, bool *coverage);
	void selectionChanged();
	void newItem(ShVarItem* item);	
	void removeSelected();
	void clearWatchList();

	void newWatchWindow();

protected:
	QModelIndexList filterSelection(const QModelIndexList& input);

private:
	QSet<ShVarItem *> watchItems;
	Ui::ShWatchDock *ui;
};


#endif // SHWATCHDOCK_H
