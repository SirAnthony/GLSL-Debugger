#ifndef SHPROXYTREEVIEW_H
#define SHPROXYTREEVIEW_H

#include <QSortFilterProxyModel>
#include "shvarmodel.h"
#include "shtreeview.h"

class ShProxyTreeView : public ShTreeView
{
	Q_OBJECT
public:
	explicit ShProxyTreeView(ShVarModel *model, QWidget *parent = 0);

protected:
	QSortFilterProxyModel* baseModel;
};


class ShBuiltInTreeView : public ShProxyTreeView
{
	Q_OBJECT
public:
	ShBuiltInTreeView(ShVarModel *model, QWidget *parent = 0);
};


class ShScopeTreeView : public ShProxyTreeView
{
	Q_OBJECT
public:
	ShScopeTreeView(ShVarModel *model, QWidget *parent = 0);
};


class ShWatchedTreeView : public ShProxyTreeView
{
	Q_OBJECT
public:
	ShWatchedTreeView(ShVarModel *model, QWidget *parent = 0);
};


class ShUniformTreeView: public ShProxyTreeView
{
	Q_OBJECT
public:
	ShUniformTreeView(ShVarModel *model, QWidget *parent = 0);
};

#endif // SHPROXYTREEVIEW_H
