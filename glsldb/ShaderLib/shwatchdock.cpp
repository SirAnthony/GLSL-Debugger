#include "shwatchdock.h"
#include "ui_shwatchdock.h"
#include "shdatamanager.h"
#include "utils/dbgprint.h"

ShWatchDock::ShWatchDock(QWidget *parent) :
	ShDockWidget(parent)
{
	ui->setupUi(this);
	ui->tvWatchList->setModel(model);

	/* Track watch items */
	connect(this->model, SIGNAL(newWatchItem(ShVarItem*)), SLOT(newItem(ShVarItem*)));
	connect(ShDataManager::get(), SIGNAL(cleanModel()), this, SLOT(clearWatchList()));
	connect(ui->WatchDelete, SIGNAL(clicked()), SLOT(removeSelected()));
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
		expand(static_cast<ShVarItem*>(item->parent()));

	ui->tvWatchList->expand(item->index());
	emit dataChanged(item->index(), item->index());
}

void ShWatchDock::updateData(CoverageMapStatus cmstatus, bool force)
{
	foreach (ShVarItem* item, watchItems) {
		bool changed = item->data(DF_CHANGED).toBool();
		enum ShVarItem::Scope scope = item->data(DF_SCOPE).toInt();
		bool builtin = item->data(DF_BUILTIN).toBool();

		dbgPrint(DBGLVL_DEBUG, ">>>>>>>>>>>>>>updateWatchListData: %s (%i, %i)\n",
				 qPrintable(item->data(DF_NAME).toString()), changed, scope);


		if (force) {
			if (scope & ShVarItem::AtScope || builtin)
				item->updateWatchData();
			else
				item->invalidateWatchData();
		} else if (scope & ShVarItem::NewInScope || (changed && scope & ShVarItem::AtScope)) {
			item->updateWatchData();
		} else if (scope & ShVarItem::LeftScope) {
			item->invalidateWatchData();
		} else if (cmstatus == COVERAGEMAP_GROWN) {
			/* If covermap grows larger, more readbacks could become possible */
			if (scope & ShVarItem::AtScope || builtin) {
				if (currentRunLevel == RL_DBG_FRAGMENT_SHADER) {
					PixelBox *dataBox = item->getPixelBoxPointer();
					if (!(dataBox->isAllDataAvailable()))
						item->updateWatchData();
				} else {
					item->updateWatchData();
				}
			} else {
				item->invalidateWatchData();
			}
		}
		/* HACK: when an error occurs in shader debugging the runlevel
			 * might change to RL_SETUP and all shader debugging data will
			 * be invalid; so we have to check it here
			 */
		if (currentRunLevel == RL_SETUP) {
			return;
		}
	}

	/* Now update all windows to update themselves if necessary */
	emit updateWindows(cmstatus != COVERAGEMAP_UNCHANGED);

	/* update model */
	QStandardItem* model_root = this->model->invisibleRootItem();
	emit this->model->dataChanged(model_root->child(0)->index(),
				model_root->child(model_root->rowCount() - 1)->index());
}

void ShWatchDock::newItem(ShVarItem *item)
{
	watchItems.insert(item);
	expand(item);
}

void ShWatchDock::removeSelected()
{
	QModelIndexList list = ui->tvWatchList->selectionModel()->selectedIndexes();
	foreach(index, list) {
		ShVarItem* item = model->itemFromIndex(index);
		watchItems.remove(item);
		model->unsetWatched(item);
	}

	updateWatchGui(cleanupSelectionList(
					ui->tvWatchList->selectionModel()->selectedRows()).count());
	emit updateWindows(false);
}


void ShWatchDock::clearWatchList()
{
	watchItems.clear();
}
