#ifndef SHVARMODEL_H
#define SHVARMODEL_H

#include <QStandardItemModel>
#include "shvaritem.h"
#include "ShaderLang.h"
#include "shuniforms.h"

class ShVarModel : public QStandardItemModel
{
	Q_OBJECT
public:
	ShVarModel(QObject *parent = 0);
	void appendRow(const ShVariableList *items);

	void setRecursive(QVariant data, varDataFields field, QStandardItem *item);
	void setChangedAndScope(const ShChangeableList* cl, DbgRsScope &scope,
			DbgRsScopeStack &stack);

	void setWatched(ShVarItem *item);
	void unsetWatched(ShVarItem *item);

	void setUniforms(const char *strings, int count);

signals:
	void addWatchItem(ShVarItem* item);

public slots:
	void valuesChanged();
	void valuesChanged(QModelIndex, QModelIndex);
	void clear();

protected:
	QList<ShUniform> uniforms;
};

#endif // SHVARMODEL_H
