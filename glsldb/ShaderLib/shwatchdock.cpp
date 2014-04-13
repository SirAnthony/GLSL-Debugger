#include "shwatchdock.h"
#include "ui_shwatchdock.h"

ShWatchDock::ShWatchDock(QWidget *parent) :
	QDockWidget(parent)
{
	ui->setupUi(this);

	/* Track watch items */
	connect(this->model, SIGNAL(newWatchItem(ShVarItem*)), SLOT(newItem(ShVarItem*));
}

ShWatchDock::~ShWatchDock()
{
	delete ui;
}

void ShWatchDock::expand(ShVarItem *item)
{
	if (!item)
		return;

	if (item->parent() != item->model()->invisibleRootItem())
		expand(item->parent());

	ui->tvWatchList->expand(item->index());
	emit dataChanged(item->index(), item->index());
}

void ShWatchDock::newItem(ShVarItem *item)
{
	watchItems.insert(item);
	expand(item);
}

void ShWatchDock::removeItems()
{
	QModelIndexList list = ui->tvWatchList->selectionModel()->selectedIndexes();
	foreach(index, list) {
		ShVarItem* item = model->itemFromIndex(index);
		watchItems.remove(item);
		model->unsetWatched(item);
	}

	updateWatchGui(
			cleanupSelectionList(tvWatchList->selectionModel()->selectedRows()).count());
}
