#ifndef SHVARMODEL_H
#define SHVARMODEL_H

#include <QStandardItemModel>
#include "shvaritem.h"
#include "ShaderLang.h"

class ShVarModel : public QStandardItemModel
{
	Q_OBJECT
public:
	ShVarModel(QObject *parent = 0);
	void appendRow(const ShVariableList *items);

	void setRecursive(QVariant data, varDataFields field, ShVarItem *item);	
	void setChangedAndScope(ShChangeableList &cl, DbgRsScope &scope,
			DbgRsScopeStack &stack);

	void setWatched(ShVarItem *item);
	void unsetWatched(ShVarItem *item);

signals:
	void addWatchItem(ShVarItem* item);

public slots:
	void valuesChanged();
	void clear();
	
};

#endif // SHVARMODEL_H
