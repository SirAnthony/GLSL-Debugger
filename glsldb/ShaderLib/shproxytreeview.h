#ifndef SHPROXYTREEVIEW_H
#define SHPROXYTREEVIEW_H

#include <QSortFilterProxyModel>
#include "models/shvarmodel.h"
#include "models/shproxymodel.h"
#include "shtreeview.h"

template <class T>
class ShTemplateTreeView : public ShTreeView
{
public:
	ShTemplateTreeView(QWidget *parent = 0) :
		ShTreeView(parent)
	{
		baseModel = NULL;
	}
	ShTemplateTreeView(ShVarModel *_model, QWidget *parent = 0) :
		ShTreeView(parent)
	{
		baseModel = NULL;
		setModel(_model);
	}
	T* model() { return baseModel; }
	virtual void setModel(ShVarModel *_model)
	{
		if (!_model)
			return;
		baseModel = new T(_model);
	}
protected:
	T* baseModel;
};


class ShProxyTreeView : public ShTemplateTreeView<ShSortFilterProxyModel>
{
public:
	ShProxyTreeView(ShVarModel *model, QWidget *parent = 0) :
		ShTemplateTreeView(model, parent) {}
};

class ShBuiltInTreeView : public ShTemplateTreeView<ShBuiltInSortFilterProxyModel>
{
public:
	ShBuiltInTreeView(ShVarModel *model, QWidget *parent = 0) :
		ShTemplateTreeView(model, parent) {}
};

class ShScopeTreeView : public ShTemplateTreeView<ShScopeSortFilterProxyModel>
{
public:
	ShScopeTreeView(ShVarModel *model, QWidget *parent = 0) :
		ShTemplateTreeView(model, parent) {}
};

class ShWatchedTreeView : public ShTemplateTreeView<ShWatchedSortFilterProxyModel>
{
public:
	ShWatchedTreeView(QWidget *parent = 0) :
		ShTemplateTreeView(parent) {}
	ShWatchedTreeView(ShVarModel *model, QWidget *parent = 0) :
		ShTemplateTreeView(model, parent) {}
};

class ShUniformTreeView: public ShTemplateTreeView<ShUniformSortFilterProxyModel>
{
public:
	ShUniformTreeView(ShVarModel *model, QWidget *parent = 0) :
		ShTemplateTreeView(model, parent) {}
};

#endif // SHPROXYTREEVIEW_H
