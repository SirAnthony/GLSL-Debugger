#include "shvaritem.h"
#include "data/vertexBox.h"
#include "data/pixelBox.h"

ShVarItem::ShVarItem(ShVariable* var, bool recursive) :
		QStandardItem()
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
	this->vertexBox = QVariant::fromValue<void*>(NULL);
	this->geometryBox = QVariant::fromValue<void*>(NULL);
	this->pixelBox = QVariant::fromValue<void*>(NULL);	
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
					row->appendRow(col);
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

ShVarItem::~ShVarItem()
{
	VertexBox* vb = this->vertexBox.value<VertexBox*>();
	VertexBox* gb = this->geometryBox.value<VertexBox*>();
	PixelBox* pb = this->pixelBox.value<PixelBox*>();
	if (vb) {
		this->vertexBox.setValue(NULL);
		delete vb;
	}
	if (gb) {
		this->geometryBox.setValue(NULL);
		delete gb;
	}
	if (pb) {
		this->pixelBox.setValue(NULL);
		delete pb;
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
	case DF_NAME:
		return this->name;
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
	case DF_DATA_PIXELBOX:
		return this->pixelBox;
	case DF_DATA_GEOMETRYBOX:
		return this->geometryBox;
	case DF_DATA_VERTEXBOX:
		return this->vertexBox;
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
		this->scope = (ShVarItem::Scope)value.toInt();
		break;
	default:
		break;
	}
}

void ShVarItem::updateWatchData()
{
	ShChangeableList cl;
	cl.numChangeables = 0;
	cl.changeables = NULL;

	ShChangeable *watchItemCgbl =  watchItem->getShChangeable();
	addShChangeable(&cl, watchItemCgbl);

	int rbFormat = watchItem->getReadbackFormat();

	if (currentRunLevel == RL_DBG_FRAGMENT_SHADER) {
		PixelBox *fb = watchItem->getPixelBoxPointer();
		if (fb) {
			if (getDebugImage(DBG_CG_CHANGEABLE, &cl, rbFormat, m_pCoverage,
							  &fb)) {
				watchItem->setCurrentValue(m_selectedPixel[0],
						m_selectedPixel[1]);
			} else {
				QMessageBox::warning(this, "Warning",
									 "The requested data could "
									 "not be retrieved.");
			}
		} else {
			if (getDebugImage(DBG_CG_CHANGEABLE, &cl, rbFormat, m_pCoverage,
							  &fb)) {
				watchItem->setPixelBoxPointer(fb);
				watchItem->setCurrentValue(m_selectedPixel[0],
						m_selectedPixel[1]);
			} else {
				QMessageBox::warning(this, "Warning",
									 "The requested data could "
									 "not be retrieved.");
			}
		}
	} else if (currentRunLevel == RL_DBG_VERTEX_SHADER) {
		VertexBox *data = new VertexBox();
		if (getDebugVertexData(DBG_CG_CHANGEABLE, &cl, m_pCoverage, data)) {
			VertexBox *vb = watchItem->getVertexBoxPointer();
			if (vb) {
				vb->addVertexBox(data);
				delete data;
			} else {
				watchItem->setVertexBoxPointer(data);
			}
			watchItem->setCurrentValue(m_selectedPixel[0]);
		} else {
			QMessageBox::warning(this, "Warning", "The requested data could "
								 "not be retrieved.");
		}
	} else if (currentRunLevel == RL_DBG_GEOMETRY_SHADER) {
		VertexBox *currentData = new VertexBox();

		UT_NOTIFY(LV_TRACE, "Get CHANGEABLE:");
		if (getDebugVertexData(DBG_CG_CHANGEABLE, &cl, m_pCoverage,
							   currentData)) {
			VertexBox *vb = watchItem->getCurrentPointer();
			if (vb) {
				vb->addVertexBox(currentData);
				delete currentData;
			} else {
				watchItem->setCurrentPointer(currentData);
			}

			VertexBox *vertexData = new VertexBox();

			UT_NOTIFY(LV_TRACE, "Get GEOMETRY_CHANGABLE:");
			if (getDebugVertexData(DBG_CG_GEOMETRY_CHANGEABLE, &cl, NULL,
								   vertexData)) {
				VertexBox *vb = watchItem->getVertexBoxPointer();
				if (vb) {
					vb->addVertexBox(vertexData);
					delete vertexData;
				} else {
					watchItem->setVertexBoxPointer(vertexData);
				}
			} else {
				QMessageBox::warning(this, "Warning",
									 "The requested data could "
									 "not be retrieved.");
			}
			watchItem->setCurrentValue(m_selectedPixel[0]);
		} else {
			QMessageBox::warning(this, "Warning", "The requested data could "
								 "not be retrieved.");
			return;
		}
	}
	freeShChangeable(&watchItemCgbl);
}
