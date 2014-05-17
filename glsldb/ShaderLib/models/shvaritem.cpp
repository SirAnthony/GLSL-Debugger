
#include "shvaritem.h"
#include "data/vertexBox.h"
#include "data/pixelBox.h"
#include "shdatamanager.h"
#include "utils/dbgprint.h"
#include "QMessageBox"


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
	this->changeable = SH_CGB_UNSET;

	// Note: Allow selecting all but samplers and batch insert into watch list.
	this->selectable = !(var->type == SH_SAMPLER);

	this->watched = false;
	this->vertexBox.setValue(new VertexBox);
	this->geometryBox.setValue(new VertexBox);
	this->pixelBox.setValue(new PixelBox);
	this->selected = QVariant();
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

ShChangeable* ShVarItem::getChangeable()
{
	ShChangeable* cgbl = NULL;
	createShChangeable(&cgbl);
	cgbl->id = this->id;
	for (int i = 0; i < 2; ++i) {
		if (this->changeableIndex[i] < 0)
			continue;
		ShChangeableIndex *index = NULL;
		createShChangeableIndex(cgbl, &index);
		index->index = this->changeableIndex[i];
		index->type = this->changeable;
	}
	return cgbl;
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
	case DF_DEBUG_SELECTED_VALUE:
		return this->selected;
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

void ShVarItem::setCurrentValue(int pixels[2], EShLanguage type)
{
	if (!watched || pixels[0] < 0 || (type == EShLangFragment && pixels[1] < 0)) {
		this->selected = QVariant();
		return;
	}

	QVariant value;
	switch (type) {
	case EShLangVertex: {
		VertexBox* vb = this->vertexBox.value<VertexBox*>();
		if (vb) {
			// FIXME: This is definitionally must not work
			Q_ASSERT(!"Stop right there");
			QVariant data;
			if (vb->getDataValue(pixels[0], &data))
				value.setValue(data.toString());
		} else {
			value.setValue("?");
		}
		break;
	}
	case EShLangGeometry: {
		VertexBox* gb = this->geometryBox.value<VertexBox*>();
		if (gb) {
			// FIXME: This is definitionally must not work
			Q_ASSERT(!"Stop right there");
			QVariant data;
			if (gb->getDataValue(pixels[0], &data))
				value.setValue(data.toString());
		} else {
			value.setValue("?");
		}
		break;
	}
	case EShLangFragment: {
		PixelBox* pb = this->pixelBox.value<PixelBox*>();
		if (pb) {
			// FIXME: This is definitionally must not work
			Q_ASSERT(!"Stop right there");
			QVariant data;
			if (pb->getDataValue(pixels[0], pixels[1], &data))
				value.setValue(data.toString());
		} else {
			value.setValue("?");
		}
		break;
	}
	default:
		break;
	}
	this->selected = value;
}

bool ShVarItem::updateWatchData(EShLanguage type)
{
	int format = this->readbackFormat();
	ShDataManager* manager = ShDataManager::get();
	ShChangeable* cgbl = this->getChangeable();
	ShChangeableList* cl = NULL;
	createShChangeableList(&cl);
	addShChangeable(cl, cgbl);
	DataBox* data = NULL;
	VertexBox* geomData = NULL;
	bool status = false;

	if (type == EShLangFragment) {
		data = this->pixelBox.value<PixelBox*>();
	} else if (type == EShLangVertex) {
		data = this->vertexBox.value<VertexBox*>();
	} else if (type == EShLangGeometry) {
		geomData = this->geometryBox.value<VertexBox*>();
		dbgPrint(DBGLVL_INFO, "Get CHANGEABLE:");
		if (manager->getDebugData(type, DBG_CG_CHANGEABLE, &cl, format, coverage, geomData)) {
			dbgPrint(DBGLVL_INFO, "Get GEOMETRY_CHANGABLE:");
			data = new VertexBox;
		}
	}

	if (data && manager->getDebugData(type, DBG_CG_CHANGEABLE, &cl,
									  format, coverage, data)) {
		status = true;
		this->setCurrentValue(manager->pixels(), type);
		if (geomData) {
			geomData->addVertexBox(data);
			delete data;
		}
	} else {
		QMessageBox::warning(this, "Warning", "The requested data could not be retrieved.");
	}

	freeShChangeable(&cgbl);
	freeShChangeableList(&cl);
	return status;
}

void ShVarItem::invalidateWatchData()
{
	VertexBox *vb = this->vertexBox.value<VertexBox*>();
	VertexBox *gb = this->geometryBox.value<VertexBox*>();
	PixelBox *pb = this->pixelBox.value<PixelBox*>();
	if (vb)
		vb->invalidateData();
	if (gb)
		gb->invalidateData();
	if (pb) {
		pb->invalidateData();
		resetValue();
	}
}

void ShVarItem::resetValue()
{
	if (watched)
		this->selected.setValue("?");
	else
		this->selected.setValue(QVariant());
}

int ShVarItem::readbackFormat()
{
	switch (this->type) {
	case SH_FLOAT:
		return GL_FLOAT;
	case SH_INT:
		return GL_INT;
	case SH_UINT:
	case SH_BOOL:
	case SH_SAMPLER:
		return GL_UNSIGNED_INT;
	case SH_STRUCT:
	default:
		dbgPrint(DBGLVL_ERROR,
				 "Could not get readback type for ShVarItem of type %d\n", this->type);
		return GL_NONE;
	}
}

bool ShVarItem::pixelDataAvaliable()
{
	return dynamic_cast<PixelBox*>(pixelBox)->isAllDataAvailable();
}
