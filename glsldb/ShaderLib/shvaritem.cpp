#include "shvaritem.h"

ShVarItem::ShVarItem(ShVariable* var, bool recursive, int index, QStandardItem *parent) :
		QStandardItem(parent)
{
	this->id = var->uniqueId;
	this->name = var->name;
	this->type = var->type;
	this->arrayType = SHV_NONE;
	this->typeString = ShGetTypeString(var);
	this->qualifier = var->qualifier;
	this->qualifierString = ShGetQualifierString(var);
	this->builtin = var->builtin;
	this->changed = false;
	this->scope = var->builtin ? InScope : OutOfScope;

	// Note: Allow selecting all but samplers and batch insert into watch list.
	this->selectable = !(var->type == SH_SAMPLER);

	this->watched = false;
	this->vertex_box = QVariant::fromValue<void*>(NULL);
	this->pixel_box = QVariant::fromValue<void*>(NULL);
	this->current_box = QVariant::fromValue<void*>(NULL);
	this->selected_value = QVariant();
	this->uniform_value = QVariant();

	if (var->structSize) {
		for (int i = 0; i < var->structSize; i++) {
			ShVarItem* elem = new ShVarItem(var->structSpec[i]);
			elem->setChangeable(SH_CGB_STRUCT, i);
			this->appendRow(elem);
		}
	}

	if (recursive) {
		// FIXME: Probably array of matrices does not work correctly.
		// If it is true, ShVariable structure must be changed
		if (var->isArray) {
			this->arrayType = SHV_ARRAY;
			for (int i = 0; i < var->arrayDepth; ++i) {
				ShVarItem *depth_elem = new ShVarItem(var, false);
				depth_elem->setChangeable(SH_CGB_ARRAY, i);
				if (var->arraySize[i] > 1) {
					for (int j = 0; j < var->arraySize[i]; ++j) {
						ShVarItem *elem = new ShVarItem(var, false);
						elem->setChangeable(SH_CGB_ARRAY, j);
						depth_elem->appendRow(elem);
					}
				}
				this->appendRow(depth_elem);
			}
		} else if (var->isMatrix) {
			this->arrayType = SHV_MATRIX;
			int elements = var->size / var->matrixColumns;
			for (int i = 0; i < elements; ++i) {
				ShVarItem *row = new ShVarItem(var, false);
				row->setChangeable(SH_CGB_ARRAY, i, 0);
				this->appendRow(row);
				for (int j = 0; j < var->matrixColumns; ++j) {
					ShVarItem *col = new ShVarItem(var, false);
					col->setChangeable(SH_CGB_ARRAY, i, j);
					row.appendRow(col);
				}
			}
		} else if (var->size > 1) {
			this->arrayType = SHV_VECTOR;
			for (int i = 0; i < var->size; ++i) {
				ShVarItem *elem = new ShVarItem(var, false);
				elem->setChangeable(SH_CGB_ARRAY, i);
				this->appendRow(elem);
			}
		}
	}
}

void ShVarItem::setChangeable(ShChangeableType t, int idxc, int idxr)
{
	this->changeable = t;
	this->changeableIndex[0] = idxc;
	this->changeableIndex[1] = idxr;
}

QVariant ShVarItem::data(int role) const
{
	if (role < DF_FIRST || role > DF_LAST)
		return QStandardItem::data(role);

	switch (role) {
	case DF_TYPE:
		return this->type;
	case DF_ARRAYTYPE:
		return this->arrayType;
	case DF_UNIQUE_ID:
		return this->id;
	case DF_QUALIFIER:
		return this->qualifier;
	case DF_QUALIFIER_STRING:
		return this->qualifierString;
	case DF_BUILTIN:
		return this->builtin;
	case DF_CHANGED:
		return this->changed;
	case DF_SCOPE:
		return this->scope;
	case DF_SELECTABLE:
		return this->selectable;
	case DF_WATCHED:
		return this->watched;
	case DF_CGBL_INDEX_B:
		return this->changeableIndex[1];
	default:
		break;
	}
}

void ShVarItem::setData(const QVariant &value, int role)
{
	if (role < DF_FIRST || role > DF_LAST) {
		QStandardItem::setData(value, role);
		return;
	}

	switch (role) {
	case DF_CHANGED:
		this->changed = value.toBool();
		break;		
	case DF_SCOPE:
		this->scope = value.toInt();
		break;
	default:
		break;
	}
}
