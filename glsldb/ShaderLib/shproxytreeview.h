#ifndef SHPROXYTREEVIEW_H
#define SHPROXYTREEVIEW_H

#include <QSortFilterProxyModel>
#include "shtreeview.h"

class ShProxyTreeView : public ShTreeView
{
	Q_OBJECT
public:
	explicit ShProxyTreeView(QSortFilterProxyModel *model, QWidget *parent = 0);

private:
	QSortFilterProxyModel* baseModel;
};


class ShBuiltInTreeView : public ShProxyTreeView
{
	Q_OBJECT
public:
	ShBuiltInTreeView(QSortFilterProxyModel *model, QObject *parent = 0);
};


class ShScopeTreeView : public ShProxyTreeView
{
	Q_OBJECT
public:
	ShScopeTreeView(QSortFilterProxyModel *model, QObject *parent = 0);
};


class ShWatchedTreeView : public ShProxyTreeView
{
	Q_OBJECT
public:
	ShWatchedTreeView(QSortFilterProxyModel *model, QObject *parent = 0);
};


class ShUniformTreeView: public ShProxyTreeView
{
	Q_OBJECT
public:
	ShUniformTreeView(QSortFilterProxyModel *model, QObject *parent = 0);
};

#endif // SHPROXYTREEVIEW_H
