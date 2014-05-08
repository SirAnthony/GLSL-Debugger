#ifndef SHPROXYMODEL_H
#define SHPROXYMODEL_H

#include <QSortFilterProxyModel>
#include "shvarmodel.h"

class ShSortFilterProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT
public:
	ShSortFilterProxyModel(ShVarModel *base, QObject *parent = 0);
};

class ShBuiltInSortFilterProxyModel : public ShSortFilterProxyModel
{
	Q_OBJECT
public:
	ShBuiltInSortFilterProxyModel(ShVarModel *base, QObject *parent = 0) :
		ShSortFilterProxyModel(base, parent) {}
protected:
	virtual bool filterAcceptsRow(int row, const QModelIndex &parent) const;
};


class ShScopeSortFilterProxyModel : public ShSortFilterProxyModel
{
	Q_OBJECT
public:
	ShScopeSortFilterProxyModel(ShVarModel *base, QObject *parent = 0) :
		ShSortFilterProxyModel(base, parent) {}
protected:
	virtual bool filterAcceptsRow(int row, const QModelIndex &parent) const;
};

class ShWatchedSortFilterProxyModel : public ShSortFilterProxyModel
{
	Q_OBJECT
public:
	ShWatchedSortFilterProxyModel(ShVarModel *base, QObject *parent = 0) :
		ShSortFilterProxyModel(base, parent) {}
protected:
	virtual bool filterAcceptsRow(int row, const QModelIndex &parent) const;
};

class ShUniformSortFilterProxyModel: public ShSortFilterProxyModel
{
Q_OBJECT
public:
	ShUniformSortFilterProxyModel(ShVarModel *base, QObject *parent = 0) :
		ShSortFilterProxyModel(base, parent) {}
protected:
	virtual bool filterAcceptsRow(int row, const QModelIndex &parent) const;
};

#endif // SHPROXYMODEL_H
