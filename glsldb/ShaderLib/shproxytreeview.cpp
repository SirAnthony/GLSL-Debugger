
#include "shproxytreeview.h"
#include "shproxymodel.h"

ShProxyTreeView::ShProxyTreeView(QSortFilterProxyModel *model, QWidget *parent) :
	ShTreeView(parent)
{
	if (!model)
		return;

	baseModel = new ShSortFilterProxyModel();
	baseModel->setSourceModel(model);
}

ShBuiltInTreeView::ShBuiltInTreeView(QSortFilterProxyModel *model, QObject *parent) :
	ShProxyTreeView(NULL, parent)
{
	baseModel = new ShBuiltInSortFilterProxyModel();
	baseModel->setSourceModel(model);
}


ShScopeTreeView::ShScopeTreeView(QSortFilterProxyModel *model, QObject *parent) :
	ShProxyTreeView(NULL, parent)
{
	baseModel = new ShScopeSortFilterProxyModel();
	baseModel->setSourceModel(model);
}


ShWatchedTreeView::ShWatchedTreeView(QSortFilterProxyModel *model, QObject *parent) :
	ShProxyTreeView(NULL, parent)
{
	baseModel = new ShWatchedSortFilterProxyModel();
	baseModel->setSourceModel(model);
}


ShUniformTreeView::ShUniformTreeView(QSortFilterProxyModel *model, QObject *parent) :
	ShProxyTreeView(NULL, parent)
{
	baseModel = new ShUniformSortFilterProxyModel();
	baseModel->setSourceModel(model);
}
