#ifndef SHVARITEM_H
#define SHVARITEM_H

#include <QStandardItem>
#include "ShaderLang.h"

typedef enum {
	SHV_NONE,
	SHV_ARRAY,
	SHV_MATRIX,
	SHV_VECTOR
} varArrayType;

typedef enum {
	DF_FIRST = Qt::UserRole + 1,
	DF_NAME,
	DF_FULLNAME,
	DF_TYPE,
	DF_TYPESTRING,
	DF_ARRAYTYPE,
	DF_QUALIFIER,
	DF_QUALIFIER_STRING,
	DF_UNIQUE_ID,
	DF_CGBL_TYPE,
	DF_CGBL_INDEX_A,
	DF_CGBL_INDEX_B,
	DF_BUILTIN,
	DF_CHANGED,
	DF_SCOPE,
	DF_SELECTABLE,
	DF_WATCHED,
	DF_DATA_PIXELBOX,
	DF_DATA_VERTEXBOX,
	DF_DATA_CURRENTBOX,
	DF_DEBUG_SELECTED_VALUE,
	DF_DEBUG_UNIFORM_VALUE,
	DF_LAST
} varDataFields;


class ShVarItem : public QStandardItem
{
	Q_OBJECT
public:
	enum Scope {
		NewInScope,
		InScope,
		InScopeStack,
		LeftScope,
		OutOfScope
	};

	ShVarItem(ShVariable *var, bool recursive = true, int index = -1, int index = -1,
			  QStandardItem *parent = 0);
	void setChangeable(ShChangeableType t, int idxc = -1, int idxr = -1);

	QVariant data(int role = DF_FIRST) const;
	void setData(const QVariant &value, int role = DF_FIRST);

	inline ShVarItem *child(int row, int column = 0) const
	{
		return QStandardItem::child(row, column);
	}



signals:

public slots:

protected:
	int id;
	bool builtin;
	bool changed;
	bool selectable;
	bool watched;
	QString name;
	variableType type;
	varArrayType arrayType;
	QString typeString;
	variableQualifier qualifier;
	QString qualifierString;
	ShChangeableType changeable;
	int changeableIndex[2];
	enum Scope scope;

	QVariant vertex_box;
	QVariant pixel_box;
	QVariant current_box;
	QVariant selected_value;
	QVariant uniform_value;
};

#endif // SHVARITEM_H
