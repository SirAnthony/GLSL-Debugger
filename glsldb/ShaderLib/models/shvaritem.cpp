
#include "shvaritem.h"
#include "data/vertexBox.h"
#include "data/pixelBox.h"
#include "utils/dbgprint.h"
#include <QtOpenGL>


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
	this->vertexBox = new VertexBox;
	this->geometryBox = new VertexBox;
	this->pixelBox = new PixelBox;
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
	if (vertexBox)
		delete vertexBox, vertexBox = NULL;
	if (geometryBox)
		delete geometryBox, geometryBox = NULL;
	if (pixelBox)
		delete pixelBox, pixelBox = NULL;
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

static inline QVariant to_qvariant(void *box)
{
	QVariant v;
	v.setValue(box);
	return v;
}

QVariant ShVarItem::data(int role) const
{
	if (role < DF_FIRST || role > DF_LAST)
		return QStandardItem::data(role);

	switch (role) {
	case DF_NAME:
		return this->name;
	case DF_FULLNAME:
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
		return to_qvariant(this->pixelBox);
	case DF_DATA_GEOMETRYBOX:
		return to_qvariant(this->geometryBox);
	case DF_DATA_VERTEXBOX:
		return to_qvariant(this->vertexBox);
	case DF_DEBUG_SELECTED_VALUE:
		return this->selected;
	default:
		break;
	}

	return QVariant();
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

void ShVarItem::setCurrentValue(const int pixels[2], ShaderMode type)
{
	if (!watched || pixels[0] < 0 || (type == smFragment && pixels[1] < 0)) {
		this->selected = QVariant();
		return;
	}

	QVariant value("?");
	DataBox* box = NULL;
	if (type == smVertex)
		box = vertexBox;
	else if (type == smGeometry)
		box = geometryBox;
	else if (type == smFragment)
		box = pixelBox;

	if (box) {
		// FIXME: This is definitionally must not work
		Q_ASSERT(!"Stop right there");
		QVariant data;
		if (box->getDataValue(pixels[0], pixels[1], &data))
			value.setValue(data.toString());
	}

	this->selected = value;
}

bool ShVarItem::updateWatchData(ShaderMode type, bool *coverage)
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

	if (type == smFragment) {
		data = pixelBox;
	} else if (type == smVertex) {
		data = vertexBox;
	} else if (type == smGeometry) {
		geomData = geometryBox;
		dbgPrint(DBGLVL_INFO, "Get CHANGEABLE:");
		if (manager->getDebugData(type, DBG_CG_CHANGEABLE, cl, format, coverage, geomData)) {
			dbgPrint(DBGLVL_INFO, "Get GEOMETRY_CHANGABLE:");
			data = new VertexBox;
		}
	}

	if (data && manager->getDebugData(type, DBG_CG_CHANGEABLE, cl,
									  format, coverage, data)) {
		status = true;
		int pixels[2];
		manager->getPixels(pixels);
		this->setCurrentValue(pixels, type);
		if (geomData) {
			geomData->addVertexBox(dynamic_cast<VertexBox *>(data));
			delete data;
		}
	} else {
		dbgPrint(DBGLVL_WARNING, "The requested data could not be retrieved.");
	}

	freeShChangeable(&cgbl);
	freeShChangeableList(&cl);
	return status;
}

void ShVarItem::invalidateWatchData()
{
	if (vertexBox)
		vertexBox->invalidateData();
	if (geometryBox)
		geometryBox->invalidateData();
	if (pixelBox) {
		pixelBox->invalidateData();
		resetValue();
	}
}

void ShVarItem::resetValue()
{
	if (watched)
		this->selected.setValue(QString("?"));
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
	return pixelBox && pixelBox->isAllDataAvailable();
}
