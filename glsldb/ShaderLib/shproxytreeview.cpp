
#include "shproxytreeview.h"
#include "shproxymodel.h"

ShProxyTreeView::ShProxyTreeView(ShVarModel *model, QWidget *parent) :
	ShTreeView(parent)
{
	baseModel = NULL;

	if (!model)
		return;

	baseModel = new ShSortFilterProxyModel(model);
}

ShBuiltInTreeView::ShBuiltInTreeView(ShVarModel *model, QWidget *parent) :
	ShProxyTreeView(NULL, parent)
{
	baseModel = new ShBuiltInSortFilterProxyModel(model);
}


ShScopeTreeView::ShScopeTreeView(ShVarModel *model, QWidget *parent) :
	ShProxyTreeView(NULL, parent)
{
	baseModel = new ShScopeSortFilterProxyModel(model);
}


ShWatchedTreeView::ShWatchedTreeView(ShVarModel *model, QWidget *parent) :
	ShProxyTreeView(NULL, parent)
{
	baseModel = new ShWatchedSortFilterProxyModel(model);
}


ShUniformTreeView::ShUniformTreeView(ShVarModel *model, QWidget *parent) :
	ShProxyTreeView(NULL, parent)
{
	baseModel = new ShUniformSortFilterProxyModel(model);
}
