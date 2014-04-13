#include "shproxymodel.h"


ShSortFilterProxyModel::ShSortFilterProxyModel(ShVarModel *base, QObject *parent) :
	QSortFilterProxyModel(parent)
{
	this->setDynamicSortFilter(true);
	this->setSourceModel(base);
}

bool ShBuiltInSortFilterProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
	QAbstractItemModel *sm = sourceModel();
	QModelIndex index = sm->index(row, 0, parent);
	return sm->data(index, DF_BUILTIN).toBool();
}

bool ShScopeSortFilterProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
	QAbstractItemModel *sm = sourceModel();
	QModelIndex index = sm->index(row, 0, parent);
	ShVarItem::Scope scope = sm->data(index, DF_SCOPE).toInt();

	/* no builtins, no uniforms, but everything else in scope */
	return !sm->data(index, DF_BUILTIN).toBool()
			&& sm->data(index, DF_QUALIFIER).toInt() != SH_UNIFORM
			&& (scope == ShVarItem::InScope || scope = ShVarItem::NewInScope
				|| scope == ShVarItem::InScopeStack);
}

bool ShWatchedSortFilterProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
	QAbstractItemModel *sm = sourceModel();
	QModelIndex index = sm->index(row, 0, parent);
	return sm->data(index, DF_WATCHED).toBool();
}


bool ShUniformSortFilterProxyModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
	QAbstractItemModel *sm = sourceModel();
	QModelIndex index = sm->index(row, 0, parent);

	/* only user declared uniforms, no built-ins */
	return sm->data(index, DF_QUALIFIER).toInt() == SH_UNIFORM
			&& !sm->data(index, DF_BUILTIN).toBool();
}
