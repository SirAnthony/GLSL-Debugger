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
	DF_DATA_GEOMETRYBOX,
	DF_DATA_VERTEXBOX,
	DF_DEBUG_SELECTED_VALUE,
	DF_DEBUG_UNIFORM_VALUE,
	DF_LAST
} varDataFields;


class ShVarItem : public QStandardItem
{
	Q_OBJECT
public:
	enum Scope {
		NewInScope = 1,
		InScope = 2,
		InScopeStack = 4,
		AtScope = 7,		// NewInScope, InScope or InScopeStack
		LeftScope = 8,
		OutOfScope = 16,
		NotAtScope = 24		// LeftScope or OutOfScope
	};

	ShVarItem(ShVariable *var, bool recursive = true);
	~ShVarItem();
	ShChangeable* getChangeable();
	void setChangeable(ShChangeableType t, int idxc = -1, int idxr = -1);

	QVariant data(int role = DF_FIRST) const;
	void setData(const QVariant &value, int role = DF_FIRST);

	void updateWatchData(EShLanguage type);
	void invalidateWatchData();

	int readbackFormat();

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

	QVariant vertexBox;
	QVariant geometryBox;
	QVariant pixelBox;
	QVariant selected;
	QVariant uniform_value;
};

#endif // SHVARITEM_H
