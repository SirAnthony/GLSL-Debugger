#include "shwatchdock.h"
#include "ui_shwatchdock.h"
#include "shdatamanager.h"
#include "watch/watchview.h"
#include "data/dataBox.h"
#include "utils/dbgprint.h"

#include <QStack>

ShWatchDock::ShWatchDock(QWidget *parent) :
	ShDockWidget(parent)
{
	ui->setupUi(this);
	ui->tvWatchList->setModel(model);

	/* Track watch items */
	connect(this->model, SIGNAL(newWatchItem(ShVarItem*)), SLOT(newItem(ShVarItem*)));
	connect(ShDataManager::get(), SIGNAL(cleanModel()), this, SLOT(clearWatchList()));
	connect(ui->WatchDelete, SIGNAL(clicked()), SLOT(removeSelected()));
	connect(ui->tvWatchList->selectionModel(),
			SIGNAL(selectionChanged(const QItemSelection &, const QItemSelection &)),
			this, SLOT(selectionChanged()));

	ShDataManager::get()->registerDock(this, ShDataManager::dmDTWatch);
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
	model->valuesChanged(item->index(), item->index());
}

void ShWatchDock::updateGui(bool enable)
{	
	bool window_active = ShDataManager::get()->hasActiveWindow();
	ui->WatchWindow->setEnabled(enable);
	ui->WatchDelete->setEnabled(enable);
	ui->WatchWindowAdd->setEnabled(enable && window_active);
}

void ShWatchDock::cleanDock(ShaderMode)
{
	ui->SelectionPos->setText("No Selection");
}

void ShWatchDock::updateData(CoverageMapStatus cmstatus, ShaderMode type, bool force)
{
	foreach (ShVarItem* item, watchItems) {
		bool changed = item->data(DF_CHANGED).toBool();
		ShVarItem::Scope scope = static_cast<ShVarItem::Scope>(item->data(DF_SCOPE).toInt());
		bool builtin = item->data(DF_BUILTIN).toBool();
		bool updated = true;

		dbgPrint(DBGLVL_DEBUG, ">>>>>>>>>>>>>>updateWatchListData: %s (%i, %i)\n",
				 qPrintable(item->data(DF_NAME).toString()), changed, scope);


		if (force) {
			if (scope & ShVarItem::AtScope || builtin)
				updated = item->updateWatchData(type);
			else
				item->invalidateWatchData();
		} else if (scope & ShVarItem::NewInScope || (changed && scope & ShVarItem::AtScope)) {
			updated = item->updateWatchData(type);
		} else if (scope & ShVarItem::LeftScope) {
			item->invalidateWatchData();
		} else if (cmstatus == COVERAGEMAP_GROWN) {
			/* If covermap grows larger, more readbacks could become possible */
			if (scope & ShVarItem::AtScope || builtin) {
				if (type == smFragment) {
					if (!item->pixelDataAvaliable())
						updated = item->updateWatchData(type);
				} else {
					updated = item->updateWatchData(type);
				}
			} else {
				item->invalidateWatchData();
			}
		}

		if (!updated)
			return;
	}

	/* Now update all windows to update themselves if necessary */
	updateWindows(cmstatus != COVERAGEMAP_UNCHANGED);

	/* update model */
	this->model->valuesChanged();
}

void ShWatchDock::resetData(ShaderMode type)
{
	if (watchItems.empty())
		return;

	foreach (ShVarItem* item, watchItems) {
		ShVarItem::Scope scope = (ShVarItem::Scope)item->data(DF_SCOPE).toInt();
		if ((scope & ShVarItem::IsInScope) || item->data(DF_BUILTIN).toBool()) {
			if (!item->updateWatchData(type))
				return;
		} else {
			item->invalidateWatchData();
		}
	}

	updateWindows(true);
	model->valuesChanged();
}

void ShWatchDock::updateCoverage(ShaderMode type, bool *coverage)
{
	if (watchItems.empty())
		return;

	int pixels[2];
	ShDataManager::get()->getPixels(&pixels);
	foreach (ShVarItem* item, watchItems) {
		varDataFields field = DF_FIRST;
		if (type == smFragment)
			field = DF_DATA_PIXELBOX;
		else if (type == smVertex)
			field = DF_DATA_VERTEXBOX;
		else if (type == smGeometry)
			field = DF_DATA_GEOMETRYBOX;

		if (field == DF_FIRST)
			continue;

		DataBox *box = static_cast<DataBox*>(item->data(field).value<void *>());
		box->setNewCoverage(coverage);
		item->setCurrentValue(pixels, type);
	}

	/* update view */
	model->valuesChanged();
}

void ShWatchDock::selectionChanged()
{
	// TODO: make it proper use of selection lists passed by signal
	QItemSelectionModel *sel_model = ui->tvWatchList->selectionModel();
	int sel_count = filterSelection(sel_model->selectedRows()).count();
	updateGui(sel_count != 0);
}

void ShWatchDock::newItem(ShVarItem *item)
{
	watchItems.insert(item);
	expand(item);
}

void ShWatchDock::removeSelected()
{
	QItemSelectionModel *sel_model = ui->tvWatchList->selectionModel();
	QModelIndexList list = sel_model->selectedIndexes();
	foreach(QModelIndex index, list) {
		ShVarItem* item = dynamic_cast<ShVarItem*>(model->itemFromIndex(index));
		watchItems.remove(item);
		model->unsetWatched(item);
	}

	int sel_count = filterSelection(sel_model->selectedRows()).count();
	updateGui(sel_count != 0);
	updateWindows(false);
}


void ShWatchDock::clearWatchList()
{
	watchItems.clear();
}

void ShWatchDock::updateSelection(int x, int y, QString &text, ShaderMode type)
{
	int pixels[2] = {x, y};
	ui->SelectionPos->setText(text);
	foreach (ShVarItem* item, watchItems)
		item->setCurrentValue(pixels, type);
}

void ShWatchDock::newWindow()
{
	int type = getWindowType();
	QList<ShVarItem*> items;
	if (!getItems(items))
		return;

	emit createWindow(items, type);
}

void ShWatchDock::extendWindow()
{
	int type = getWindowType();
	QList<ShVarItem*> items;
	if (!getItems(items))
		return;

	emit extendWindow(items, type);
}

int ShWatchDock::getWindowType()
{
	ShWindowManager::WindowType type = ShWindowManager::wtNone;
	ShaderMode lang = ShDataManager::get()->getMode();
	if (lang == smFragment)
		type = ShWindowManager::wtFragment;
	else if (lang == smVertex)
		type = ShWindowManager::wtVertex;
	else if (lang == smGeometry)
		type = ShWindowManager::wtGeometry;
	return type;
}

int ShWatchDock::getItems(QList<ShVarItem *> &items)
{
	QItemSelectionModel *sel_model = ui->tvWatchList->selectionModel();
	QModelIndexList list = filterSelection(sel_model->selectedRows(0));

	foreach(QModelIndex index, list) {
		ShVarItem* item = dynamic_cast<ShVarItem *>(model->itemFromIndex(index));
		items.append(item);
	}

	return items.count();
}

QModelIndexList ShWatchDock::filterSelection(const QModelIndexList &input)
{
	QModelIndexList out;         // Resulting filtered list.
	QStack<QModelIndex> stack;      // For iterative tree traversal.

	if (!model)
		return out;

	/*
	 * Add directly selected items in reverse order such that getting them
	 * from the stack restores the original order. This is also required
	 * for all following push operations.
	 */
	for (int i = input.count() - 1; i >= 0; i--)
		stack.push(input[i]);

	while (!stack.isEmpty()) {
		QModelIndex idx = stack.pop();
		if (!idx.isValid())
			continue;

		// Not sure if it right model
		ShVarItem *item = dynamic_cast<ShVarItem*>(model->itemFromIndex(idx));
		for (int c = item->rowCount() - 1; c >= 0; --c)
			stack.push(idx.child(c, idx.column()));

		if (!item->rowCount() && item->isSelectable() && !out.contains(idx))
			out << idx;
	}

	return out;
}
