/******************************************************************************

Copyright (C) 2006-2009 Institute for Visualization and Interactive Systems
(VIS), Universität Stuttgart.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice, this
    list of conditions and the following disclaimer in the documentation and/or
    other materials provided with the distribution.

  * Neither the name of the name of VIS, Universität Stuttgart nor the names
    of its contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#include "shVarModel.qt.h"

#include <QtGui/QSortFilterProxyModel>
#include <QtCore/QEventLoop>
#include <QtCore/QStack>

#include "dbgprint.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include "GL/gl.h"

typedef enum {
	DF_NAME = 0,
	DF_FULLNAME,
	DF_TYPE,
	DF_TYPEID,
	DF_QUALIFIER,
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

OldShVarItem::OldShVarItem(const QList<QVariant> &i_qData, OldShVarItem *i_qParent)
{
	m_qData = i_qData;
	m_qParent = i_qParent;
}

OldShVarItem::~OldShVarItem()
{
	qDeleteAll(m_qChilds);
}

void OldShVarItem::appendChild(OldShVarItem *i_qChild)
{
	m_qChilds.append(i_qChild);
}

OldShVarItem* OldShVarItem::child(int i_nRow)
{
	return m_qChilds.value(i_nRow);

}

OldShVarItem *OldShVarItem::parent()
{
	return m_qParent;
}

int OldShVarItem::childCount() const
{
	return m_qChilds.count();
}

int OldShVarItem::columnCount() const
{
	return m_qData.count();
}

int OldShVarItem::row() const
{
	if (m_qParent) {
		return m_qParent->m_qChilds.indexOf(const_cast<OldShVarItem*>(this));
	}
	return 0;
}

QVariant OldShVarItem::data(int i_nColumn) const
{
	return m_qData.value(i_nColumn);
}

void OldShVarItem::setData(int i_nColumn, QVariant i_qData)
{
	m_qData[i_nColumn] = i_qData;
}

ShChangeable* OldShVarItem::getShChangeable(void)
{
	ShChangeable *cgbl = NULL;

	if (data(DF_UNIQUE_ID).toInt() != -1) {
		cgbl = createShChangeable(data(DF_UNIQUE_ID).toInt());
	} else {
		cgbl = m_qParent->getShChangeable();
		ShChangeableIndex* index;
		index = createShChangeableIndex(
				(ShChangeableType) (data(DF_CGBL_TYPE).toInt()),
				data(DF_CGBL_INDEX_A).toInt());
		addShIndexToChangeable(cgbl, index);
		if (data(DF_CGBL_INDEX_B).isValid()) {
			ShChangeableIndex* matIdx;
			matIdx = createShChangeableIndex(
					(ShChangeableType) (data(DF_CGBL_TYPE).toInt()),
					data(DF_CGBL_INDEX_B).toInt());
			addShIndexToChangeable(cgbl, matIdx);
		}
	}

	return cgbl;
}

void OldShVarItem::setPixelBoxPointer(PixelBox *fb)
{
	if (fb) {
		dbgPrint(DBGLVL_DEBUG,
				"setPixelBoxPointer: PixelBox %p w=%i h=%i dmap=%p coverage=%p\n", fb, fb->getWidth(), fb->getHeight(), fb->getDataMapPointer(), fb->getCoveragePointer());
	} else {
		dbgPrint(DBGLVL_DEBUG, "setPixelBoxPointer: NULL\n");
	}
	setData(DF_DATA_PIXELBOX, QVariant::fromValue<void*>((void*) fb));
}

PixelBox* OldShVarItem::getPixelBoxPointer(void)
{
	QVariant v = data(DF_DATA_PIXELBOX);
	if (v.value<void*>() == NULL) {
		dbgPrint(DBGLVL_DEBUG, "getPixelBoxPointer: PixelBox not set\n");
		return NULL;
	} else {
		dbgPrint(DBGLVL_DEBUG,
				"getPixelBoxPointer: PixelBox %p\n", v.value<void*>());
		return (PixelBox*) v.value<void*>();
	}
}

void OldShVarItem::setVertexBoxPointer(VertexBox *vb)
{
	if (vb) {
		dbgPrint(DBGLVL_DEBUG,
				"setVertexBoxPointer: VertexBox %p numVertices=%i data=%p\n", vb, vb->getNumVertices(), vb->getDataPointer());
	} else {
		dbgPrint(DBGLVL_DEBUG, "setVertexBoxPointer: NULL\n");
	}
	setData(DF_DATA_VERTEXBOX, QVariant::fromValue<void*>((void*) vb));
}

VertexBox* OldShVarItem::getVertexBoxPointer(void)
{
	QVariant v = data(DF_DATA_VERTEXBOX);
	if (v.value<void*>() == NULL) {
		dbgPrint(DBGLVL_DEBUG, "getVertexBoxPointer: VertexBox not set\n");
		return NULL;
	} else {
		return (VertexBox*) v.value<void*>();
	}
}

void OldShVarItem::setCurrentPointer(VertexBox *vb)
{
	if (vb) {
		dbgPrint(DBGLVL_DEBUG,
				"setCurrentPointer: VertexBox %p numVertices=%i data=%p\n", vb, vb->getNumVertices(), vb->getDataPointer());
	} else {
		dbgPrint(DBGLVL_DEBUG, "setCurrentPointer: NULL\n");
	}
	setData(DF_DATA_CURRENTBOX, QVariant::fromValue<void*>((void*) vb));
}

VertexBox* OldShVarItem::getCurrentPointer(void)
{
	QVariant v = data(DF_DATA_CURRENTBOX);
	if (v.value<void*>() == NULL) {
		dbgPrint(DBGLVL_DEBUG, "getCurrentPointer: VertexBox not set\n");
		return NULL;
	} else {
		return (VertexBox*) v.value<void*>();
	}
}

bool OldShVarItem::isChanged(void)
{
	return data(DF_CHANGED).toBool();
}

bool OldShVarItem::isSelectable(void)
{
	return data(DF_SELECTABLE).toBool();
}

bool OldShVarItem::isInScope(void)
{
	return data(DF_SCOPE).toInt() == InScope
			|| data(DF_SCOPE).toInt() == NewInScope;
}

bool OldShVarItem::hasEnteredScope(void)
{
	return data(DF_SCOPE).toInt() == NewInScope;
}

bool OldShVarItem::hasLeftScope(void)
{
	return data(DF_SCOPE).toInt() == LeftScope;
}

bool OldShVarItem::isInScopeStack(void)
{
	return data(DF_SCOPE).toInt() == InScopeStack;
}

bool OldShVarItem::isBuildIn(void)
{
	return data(DF_BUILTIN).toBool();
}

bool OldShVarItem::isUniform(void)
{
	return data(DF_QUALIFIER).toString() == "uniform";
}

bool OldShVarItem::isActiveUniform(void)
{
	return isUniform() && data(DF_DEBUG_UNIFORM_VALUE) != QVariant();
}

QString OldShVarItem::getFullName(void)
{
	return data(DF_FULLNAME).toString();
}

int OldShVarItem::getReadbackFormat()
{
	int varType = data(DF_TYPE).toInt();

	switch (varType) {
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
		dbgPrint( DBGLVL_ERROR,
				"Could not get readback type for ShVarItem of type %d\n", varType);
		return GL_NONE;
	}
}

OldShVarModel::OldShVarModel(ShVariableList *i_pVL, QObject *i_qParent,
		QCoreApplication *i_qApp) :
		QAbstractItemModel(i_qParent)
{
	QList<QVariant> rootData;
	rootData << "Name" << "FullName" << "Type" << "TypeID" << "Qualifier"
			<< "ID" << "CgblType" << "CgblIndexA" << "CgblIndexB" << "Built-In"
			<< "Changed" << "Scope" << "Selectable" << "Watched"
			<< "Data PixelBox" << "Data VertexBox" << "Data CurrentBox"
			<< "Selection Value" << "Value";
	m_qRootItem = new OldShVarItem(rootData);
	setupModelData(i_pVL, m_qRootItem);

	m_qAllProxy = new QSortFilterProxyModel(this);
	m_qAllProxy->setDynamicSortFilter(true);
	m_qAllProxy->setSourceModel(this);

	m_qBuiltInProxy = new QSortFilterProxyModel(this);
	m_qBuiltInProxy->setDynamicSortFilter(true);
	m_qBuiltInProxy->setFilterRegExp(QRegExp("(true|1)"));
	m_qBuiltInProxy->setFilterKeyColumn(DF_BUILTIN);
	m_qBuiltInProxy->setSourceModel(this);

	m_qScopeProxy = new ScopeSortFilterProxyModel(this);
	m_qScopeProxy->setDynamicSortFilter(true);
	m_qScopeProxy->setSourceModel(this);

	// do no show a special sampler tab. This is confusing in case samplers
	// are defined in a uniform struct
#if 0
	m_qSamplerProxy = new SamplerSortFilterProxyModel(this);
	m_qSamplerProxy->setDynamicSortFilter(true);
	m_qSamplerProxy->setSourceModel(this);
#endif

	m_qWatchProxy = new QSortFilterProxyModel(this);
	m_qWatchProxy->setDynamicSortFilter(true);
	m_qWatchProxy->setFilterRegExp(QRegExp("(true|1)"));
	m_qWatchProxy->setFilterKeyColumn(DF_WATCHED);
	m_qWatchProxy->setSourceModel(this);

	m_qUniformProxy = new UniformSortFilterProxyModel(this);
	m_qUniformProxy->setDynamicSortFilter(true);
	m_qUniformProxy->setSourceModel(this);

	m_qApp = i_qApp;
}

OldShVarModel::~OldShVarModel()
{
	delete m_qRootItem;
}

QModelIndex OldShVarModel::index(int i_nRow, int i_nColumn,
		const QModelIndex &i_qParent) const
{
	OldShVarItem *parentItem;

	if (!i_qParent.isValid()) {
		parentItem = m_qRootItem;
	} else {
		parentItem = static_cast<OldShVarItem*>(i_qParent.internalPointer());
	}

	OldShVarItem *childItem = parentItem->child(i_nRow);
	if (childItem) {
		return createIndex(i_nRow, i_nColumn, childItem);
	} else {
		return QModelIndex();
	}
}

QModelIndex OldShVarModel::getIndex(OldShVarItem *i_qItem, int i_nColumn)
{
	if (i_qItem->parent() != m_qRootItem) {
		return OldShVarModel::index(i_qItem->row(), i_nColumn,
				getIndex(i_qItem->parent(), i_nColumn));
	} else {
		return OldShVarModel::index(i_qItem->row(), i_nColumn);
	}
}

QModelIndex OldShVarModel::parent(const QModelIndex &i_qIndex) const
{
	if (!i_qIndex.isValid()) {
		return QModelIndex();
	}

	OldShVarItem *childItem = static_cast<OldShVarItem*>(i_qIndex.internalPointer());
	OldShVarItem *parentItem = childItem->parent();

	if (parentItem == m_qRootItem) {
		return QModelIndex();
	}

	return createIndex(parentItem->row(), 0, parentItem);
}

int OldShVarModel::rowCount(const QModelIndex &i_qParent) const
{
	OldShVarItem *parentItem;

	if (!i_qParent.isValid()) {
		parentItem = m_qRootItem;
	} else {
		parentItem = static_cast<OldShVarItem*>(i_qParent.internalPointer());
	}

	return parentItem->childCount();
}

int OldShVarModel::columnCount(const QModelIndex &i_qParent) const
{
	if (i_qParent.isValid()) {
		return static_cast<OldShVarItem*>(i_qParent.internalPointer())->columnCount();
	} else {
		return m_qRootItem->columnCount();
	}
}

QVariant OldShVarModel::data(const QModelIndex &i_qIndex, int i_nRole) const
{
	if (!i_qIndex.isValid()) {
		return QVariant();
	}

	OldShVarItem *item = static_cast<OldShVarItem*>(i_qIndex.internalPointer());

	switch (i_nRole) {
	case Qt::DisplayRole:
		return item->data(i_qIndex.column());
	case Qt::TextColorRole:
		if (item->data(DF_CHANGED).toBool() == true) {
			return Qt::red;
		} else {
			return QVariant();
		}
	case Qt::FontRole:
		// all inactive uniforms should never be made bold
		if (item->isUniform() && !item->isActiveUniform()) {
			return QVariant();
		}
		if (item->isInScope()) {
			QFont f;
			f.setWeight(QFont::DemiBold);
			return f;
		} else {
			return QVariant();
		}
	default:
		return QVariant();
	}

	//return item->data(i_qIndex.column());
	return QVariant();
}

Qt::ItemFlags OldShVarModel::flags(const QModelIndex &i_qIndex) const
{
	if (!i_qIndex.isValid()) {
		return Qt::ItemIsEnabled;
	}

	OldShVarItem *item = static_cast<OldShVarItem*>(i_qIndex.internalPointer());
	if (item->data(DF_SELECTABLE).toBool() == true) {
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	} else {
		return Qt::ItemIsEnabled;
	}
}

QVariant OldShVarModel::headerData(int i_nSection, Qt::Orientation i_qOrientation,
		int i_nRole) const
{
	if (i_qOrientation == Qt::Horizontal && i_nRole == Qt::DisplayRole) {
		return m_qRootItem->data(i_nSection);
	}

	return QVariant();
}

void OldShVarModel::clearChanged(OldShVarItem *i_qItem)
{
	/* Hint: this does not emit any dataChanged signal */

	int j;

	if (i_qItem != m_qRootItem) {
		i_qItem->setData(DF_CHANGED, false);
	}

	for (j = 0; j < i_qItem->childCount(); j++) {
		clearChanged(i_qItem->child(j));
	}
}

void OldShVarModel::setRecursiveScope(OldShVarItem *i_qItem, OldShVarItem::Scope scope)
{
	int j;

	if (!i_qItem)
		return;

	i_qItem->setData(DF_SCOPE, scope);

	for (j = 0; j < i_qItem->childCount(); j++) {
		setRecursiveScope(i_qItem->child(j), scope);
	}
}

void OldShVarModel::setRecursiveChanged(OldShVarItem *i_qItem)
{
	int j;

	if (!i_qItem)
		return;

	i_qItem->setData(DF_CHANGED, true);

	for (j = 0; j < i_qItem->childCount(); j++) {
		setRecursiveChanged(i_qItem->child(j));
	}
}

void OldShVarModel::setChangedAndScope(ShChangeableList &i_pCL, DbgRsScope &i_pSL,
		DbgRsScope &i_pSSL)
{
	int i, j, l, m;

	clearChanged(m_qRootItem);

	dumpShChangeableList(&i_pCL);

	/* Changed? */
	for (i = 0; i < i_pCL.numChangeables; i++) {
		ShChangeable *c = i_pCL.changeables[i];

		for (j = 0; j < m_qRootItem->childCount(); j++) {
			QVariant id = m_qRootItem->child(j)->data(DF_UNIQUE_ID);
			if (id == QVariant(c->id)) {
				OldShVarItem *item = m_qRootItem->child(j);
				item->setData(DF_CHANGED, true);

				for (l = 0; l < c->numIndices; l++) {
					switch (c->indices[l]->type) {
					case SH_CGB_ARRAY:
					case SH_CGB_STRUCT:
						if (c->indices[l]->index == -1) {
							item->setData(DF_CHANGED, true);
							goto ENDINDICES;
						} else {
							if (item->data(DF_CGBL_INDEX_B).isValid()) {
								if ((c->numIndices - l) >= 2) {
									item =
											item->child(
													c->indices[l]->index
															* item->data(
																	DF_CGBL_INDEX_B).toInt()
															+ c->indices[l + 1]->index);
									l++;
								}
							} else {
								item = item->child(c->indices[l]->index);
								item->setData(DF_CHANGED, true);
							}
						}
						break;
					case SH_CGB_SWIZZLE:
						for (m = 0; m < 4; m++) {
							if ((c->indices[l]->index & (1 << m)) == (1 << m)) {
								item->child(m)->setData(DF_CHANGED, true);
							}
						}
						item = NULL;
						break;
					default:
						break;
					}
				}
				ENDINDICES: setRecursiveChanged(item);
			}
		}
	}

	/* check scope */
	for (j = 0; j < m_qRootItem->childCount(); j++) {
		OldShVarItem *item = m_qRootItem->child(j);
		if (item->isBuildIn()) {
			continue;
		}
		QVariant id = item->data(DF_UNIQUE_ID);
		OldShVarItem::Scope oldScope =
				(OldShVarItem::Scope) item->data(DF_SCOPE).toInt();
		/* check scope list */
		for (i = 0; i < i_pSL.numIds; i++) {
			int varId = i_pSL.ids[i];
			if (id == varId) {
				if (oldScope == OldShVarItem::InScope) {
				} else if (oldScope == OldShVarItem::NewInScope) {
					setRecursiveScope(item, OldShVarItem::InScope);
				} else {
					setRecursiveScope(item, OldShVarItem::NewInScope);
				}
				break;
			}
		}
		/* not in scope list */
		if (i == i_pSL.numIds) {
			/* check scope stack */
			for (i = 0; i < i_pSSL.numIds; i++) {
				int varId = i_pSSL.ids[i];
				if (id == varId) {
					setRecursiveScope(item, OldShVarItem::InScopeStack);
					break;
				}
			}
			/* not in scope stack */
			if (i == i_pSSL.numIds) {
				if (oldScope == OldShVarItem::LeftScope) {
					setRecursiveScope(item, OldShVarItem::OutOfScope);
				} else {
					setRecursiveScope(item, OldShVarItem::LeftScope);
				}
			}
		}
	}

	OldShVarItem *item;
	item = m_qRootItem->child(0);
	QModelIndex indexBegin = OldShVarModel::index(item->row(), DF_NAME);
	item = m_qRootItem->child(m_qRootItem->childCount() - 1);
	QModelIndex indexEnd = OldShVarModel::index(item->row(), DF_LAST - 1);
	emit dataChanged(indexBegin, indexEnd);
}

void OldShVarItem::setCurrentValue(int key0, int key1)
{
	if (key0 >= 0 && key1 >= 0 && data(DF_WATCHED).toBool()) {
		if (data(DF_DATA_PIXELBOX).value<void*>() != NULL) {
			PixelBox *fb = getPixelBoxPointer();
			if (fb) {
				QVariant value;
				bool validValue = fb->getDataValue(key0, key1, &value);
				if (validValue) {
					setData(DF_DEBUG_SELECTED_VALUE, value.toString());
					return;
				}
			}
		}
		setData(DF_DEBUG_SELECTED_VALUE, "?");
		return;
	}
	setData(DF_DEBUG_SELECTED_VALUE, QVariant());
}

void OldShVarItem::setCurrentValue(int key0)
{
	if (key0 >= 0 && data(DF_WATCHED).toBool()) {
		if (data(DF_DATA_CURRENTBOX).value<void*>() != NULL) {
			VertexBox *vb = getCurrentPointer();
			if (vb) {
				QVariant value;
				bool validValue = vb->getDataValue(key0, &value);
				if (validValue) {
					setData(DF_DEBUG_SELECTED_VALUE, value.toString());
					return;
				}
			}
		} else if (data(DF_DATA_VERTEXBOX).value<void*>() != NULL) {
			VertexBox *vb = getVertexBoxPointer();
			if (vb) {
				QVariant value;
				bool validValue = vb->getDataValue(key0, &value);
				if (validValue) {
					setData(DF_DEBUG_SELECTED_VALUE, value.toString());
					return;
				}
			}
		}
		setData(DF_DEBUG_SELECTED_VALUE, "?");
		return;
	}
	setData(DF_DEBUG_SELECTED_VALUE, QVariant());
}

void OldShVarItem::resetCurrentValue(void)
{
	if (data(DF_WATCHED).toBool()) {
		setData(DF_DEBUG_SELECTED_VALUE, "?");
	} else {
		setData(DF_DEBUG_SELECTED_VALUE, QVariant());
	}
}

void OldShVarModel::setCurrentValues(int key0, int key1)
{
	for (int j = 0; j < m_qWatchListItems.count(); j++) {
		m_qWatchListItems[j]->setCurrentValue(key0, key1);
	}

	currentValuesChanged();
}

void OldShVarModel::setCurrentValues(int key0)
{
	for (int j = 0; j < m_qWatchListItems.count(); j++) {
		m_qWatchListItems[j]->setCurrentValue(key0);
	}

	currentValuesChanged();
}

void OldShVarModel::resetCurrentValues(void)
{
	for (int j = 0; j < m_qWatchListItems.count(); j++) {
		m_qWatchListItems[j]->resetCurrentValue();
	}

	currentValuesChanged();
}

void OldShVarModel::currentValuesChanged(void)
{
	/* TODO: is there a way to notify the view more efficiently? I.e . how to
	 * access a single column?
	 */
	OldShVarItem *item;
	item = m_qRootItem->child(0);
	QModelIndex indexBegin = OldShVarModel::index(item->row(), DF_NAME);
	item = m_qRootItem->child(m_qRootItem->childCount() - 1);
	QModelIndex indexEnd = OldShVarModel::index(item->row(), DF_LAST - 1);
	emit dataChanged(indexBegin, indexEnd);
}

Uniform::Uniform()
{
	m_data = NULL;
	m_isVector = false;
	m_isMatrix = false;
	m_arraySize = 0;
	m_rows = 0;
	m_columns = 0;
}

Uniform::~Uniform()
{
	delete m_data;
}

int Uniform::initialize(const char* serializedUniform)
{
	GLint nameLength, valueSize, arraySize;
	GLuint type;
	// char *pName;
	int currentOffset;

	currentOffset = 0;
	memcpy(&nameLength, serializedUniform, sizeof(GLint));
	currentOffset += sizeof(GLint);
	m_name = QString::fromAscii(serializedUniform + currentOffset, nameLength);
	currentOffset += nameLength;
	memcpy(&type, serializedUniform + currentOffset, sizeof(GLuint));
	currentOffset += sizeof(GLuint);
	memcpy(&arraySize, serializedUniform + currentOffset, sizeof(GLint));
	m_arraySize = arraySize;
	currentOffset += sizeof(GLint);
	memcpy(&valueSize, serializedUniform + currentOffset, sizeof(GLint));
	currentOffset += sizeof(GLint);

	switch (type) {
	case GL_FLOAT:
		m_data = new TypedUniformData<GLfloat>(
				serializedUniform + currentOffset, 1);
		m_rows = 1;
		m_columns = 1;
		break;
	case GL_FLOAT_VEC2:
		m_data = new TypedUniformData<GLfloat>(
				serializedUniform + currentOffset, 2);
		m_isVector = true;
		m_rows = 1;
		m_columns = 2;
		break;
	case GL_FLOAT_VEC3:
		m_data = new TypedUniformData<GLfloat>(
				serializedUniform + currentOffset, 3);
		m_isVector = true;
		m_rows = 1;
		m_columns = 3;
		break;
	case GL_FLOAT_VEC4:
		m_data = new TypedUniformData<GLfloat>(
				serializedUniform + currentOffset, 4);
		m_isVector = true;
		m_rows = 1;
		m_columns = 4;
		break;
	case GL_FLOAT_MAT2:
		m_data = new TypedUniformData<GLfloat>(
				serializedUniform + currentOffset, 4);
		m_isMatrix = true;
		m_rows = 2;
		m_columns = 2;
		break;
	case GL_FLOAT_MAT2x3:
		m_data = new TypedUniformData<GLfloat>(
				serializedUniform + currentOffset, 6);
		m_isMatrix = true;
		m_rows = 2;
		m_columns = 3;
		break;
	case GL_FLOAT_MAT2x4:
		m_data = new TypedUniformData<GLfloat>(
				serializedUniform + currentOffset, 8);
		m_isMatrix = true;
		m_rows = 2;
		m_columns = 4;
		break;
	case GL_FLOAT_MAT3:
		m_data = new TypedUniformData<GLfloat>(
				serializedUniform + currentOffset, 9);
		m_isMatrix = true;
		m_rows = 3;
		m_columns = 3;
		break;
	case GL_FLOAT_MAT3x2:
		m_data = new TypedUniformData<GLfloat>(
				serializedUniform + currentOffset, 6);
		m_isMatrix = true;
		m_rows = 3;
		m_columns = 2;
		break;
	case GL_FLOAT_MAT3x4:
		m_data = new TypedUniformData<GLfloat>(
				serializedUniform + currentOffset, 12);
		m_isMatrix = true;
		m_rows = 3;
		m_columns = 4;
		break;
	case GL_FLOAT_MAT4:
		m_data = new TypedUniformData<GLfloat>(
				serializedUniform + currentOffset, 16);
		m_isMatrix = true;
		m_rows = 4;
		m_columns = 4;
		break;
	case GL_FLOAT_MAT4x2:
		m_data = new TypedUniformData<GLfloat>(
				serializedUniform + currentOffset, 8);
		m_isMatrix = true;
		m_rows = 4;
		m_columns = 2;
		break;
	case GL_FLOAT_MAT4x3:
		m_data = new TypedUniformData<GLfloat>(
				serializedUniform + currentOffset, 12);
		m_isMatrix = true;
		m_rows = 4;
		m_columns = 3;
		break;
	case GL_INT:
		m_data = new TypedUniformData<GLint>(serializedUniform + currentOffset,
				1);
		m_rows = 1;
		m_columns = 1;
		break;
	case GL_INT_VEC2:
		m_data = new TypedUniformData<GLint>(serializedUniform + currentOffset,
				2);
		m_isVector = true;
		m_rows = 1;
		m_columns = 2;
		break;
	case GL_INT_VEC3:
		m_data = new TypedUniformData<GLint>(serializedUniform + currentOffset,
				3);
		m_isVector = true;
		m_rows = 1;
		m_columns = 3;
		break;
	case GL_INT_VEC4:
		m_data = new TypedUniformData<GLint>(serializedUniform + currentOffset,
				4);
		m_isVector = true;
		m_rows = 1;
		m_columns = 4;
		break;
	case GL_SAMPLER_1D:
	case GL_SAMPLER_2D:
	case GL_SAMPLER_3D:
	case GL_SAMPLER_CUBE:
	case GL_SAMPLER_1D_SHADOW:
	case GL_SAMPLER_2D_SHADOW:
	case GL_SAMPLER_2D_RECT_ARB:
	case GL_SAMPLER_2D_RECT_SHADOW_ARB:
	case GL_SAMPLER_1D_ARRAY_EXT:
	case GL_SAMPLER_2D_ARRAY_EXT:
	case GL_SAMPLER_BUFFER_EXT:
	case GL_SAMPLER_1D_ARRAY_SHADOW_EXT:
	case GL_SAMPLER_2D_ARRAY_SHADOW_EXT:
	case GL_SAMPLER_CUBE_SHADOW_EXT:
	case GL_INT_SAMPLER_1D_EXT:
	case GL_INT_SAMPLER_2D_EXT:
	case GL_INT_SAMPLER_3D_EXT:
	case GL_INT_SAMPLER_CUBE_EXT:
	case GL_INT_SAMPLER_2D_RECT_EXT:
	case GL_INT_SAMPLER_1D_ARRAY_EXT:
	case GL_INT_SAMPLER_2D_ARRAY_EXT:
	case GL_INT_SAMPLER_BUFFER_EXT:
	case GL_UNSIGNED_INT_SAMPLER_1D_EXT:
	case GL_UNSIGNED_INT_SAMPLER_2D_EXT:
	case GL_UNSIGNED_INT_SAMPLER_3D_EXT:
	case GL_UNSIGNED_INT_SAMPLER_CUBE_EXT:
	case GL_UNSIGNED_INT_SAMPLER_2D_RECT_EXT:
	case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY_EXT:
	case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY_EXT:
	case GL_UNSIGNED_INT_SAMPLER_BUFFER_EXT:
		m_data = new TypedUniformData<GLint>(serializedUniform + currentOffset,
				1);
		m_rows = 1;
		m_columns = 1;
		break;
	case GL_BOOL:
		m_data = new TypedUniformData<GLboolean>(
				serializedUniform + currentOffset, 1);
		m_rows = 1;
		m_columns = 1;
		break;
	case GL_BOOL_VEC2:
		m_data = new TypedUniformData<GLboolean>(
				serializedUniform + currentOffset, 2);
		m_isVector = true;
		m_rows = 1;
		m_columns = 2;
		break;
	case GL_BOOL_VEC3:
		m_data = new TypedUniformData<GLboolean>(
				serializedUniform + currentOffset, 3);
		m_isVector = true;
		m_rows = 1;
		m_columns = 3;
		break;
	case GL_BOOL_VEC4:
		m_data = new TypedUniformData<GLboolean>(
				serializedUniform + currentOffset, 4);
		m_isVector = true;
		m_rows = 1;
		m_columns = 4;
		break;
	case GL_UNSIGNED_INT:
		m_data = new TypedUniformData<GLuint>(serializedUniform + currentOffset,
				1);
		m_rows = 1;
		m_columns = 1;
		break;
	case GL_UNSIGNED_INT_VEC2_EXT:
		m_data = new TypedUniformData<GLuint>(serializedUniform + currentOffset,
				2);
		m_isVector = true;
		m_rows = 1;
		m_columns = 2;
		break;
	case GL_UNSIGNED_INT_VEC3_EXT:
		m_data = new TypedUniformData<GLuint>(serializedUniform + currentOffset,
				3);
		m_isVector = true;
		m_rows = 1;
		m_columns = 3;
		break;
	case GL_UNSIGNED_INT_VEC4_EXT:
		m_data = new TypedUniformData<GLuint>(serializedUniform + currentOffset,
				4);
		m_isVector = true;
		m_rows = 1;
		m_columns = 4;
		break;
	default:
		dbgPrint(DBGLVL_ERROR, "HMM, unkown shader variable type: %i\n", type);
		m_data = NULL;
	}

	dbgPrint(DBGLVL_INFO, "Got Uniform " "%s" ":\n", m_name.toAscii().data());
	dbgPrint(DBGLVL_INFO, "    arraySize: %d\n", m_arraySize);
	dbgPrint(DBGLVL_INFO, "    isVector: %d\n", m_isVector);
	dbgPrint(DBGLVL_INFO, "    isMatrix: %d\n", m_isMatrix);
	dbgPrint(DBGLVL_INFO, "    rows: %d\n", m_rows);
	dbgPrint(DBGLVL_INFO, "    columns: %d\n", m_columns);

	currentOffset += valueSize;

	return currentOffset;
}

QString Uniform::toString() const
{
	QString result;

	for (int k = 0; k < m_arraySize; ++k) {
		if (k > 0) {
			result += ", ";
		}
		result += toString(k);
	}
	return result;
}

QString Uniform::toString(int arrayElement) const
{
	if (arrayElement > m_arraySize) {
		return "?";
	} else {
		QString result;
		if (m_isMatrix) {
			result += '(';
		}
		for (int j = 0; j < m_rows; ++j) {
			if (j != 0) {
				result += ", ";
			}
			if (m_isMatrix || m_isVector) {
				result += '(';
			}
			for (int i = 0; i < m_columns; ++i) {
				if (i != 0) {
					result += ", ";
				}
				result += m_data->toString(
						m_columns * (arrayElement * m_rows + j) + i);
			}
			if (m_isMatrix || m_isVector) {
				result += ')';
			}
		}
		if (m_isMatrix) {
			result += ')';
		}
		return result;
	}
}

void OldShVarModel::setUniformValues(char *pSerializedUniforms, int numUniforms)
{
	char *p = pSerializedUniforms;

	for (int i = 0; i < numUniforms; ++i) {
		Uniform u;
		p += u.initialize(p);

		for (int j = 0; j < m_qRootItem->childCount(); j++) {
			OldShVarItem *item = m_qRootItem->child(j);
			if (item->isBuildIn() || !item->isUniform()) {
				continue;
			}
			setRecursiveUniformValues(item, u);
		}
	}
}

void OldShVarModel::setRecursiveUniformValues(OldShVarItem *item, const Uniform& u)
{
	if (!item)
		return;

	if (item->getFullName() == u.name()) {
		dbgPrint(DBGLVL_INFO, "found uniform: %s\n", u.name().toAscii().data());
		item->setData(DF_DEBUG_UNIFORM_VALUE, u.toString());
		emit dataChanged(OldShVarModel::getIndex(item, 0),
				OldShVarModel::getIndex(item, DF_LAST - 1));
	} else {
		for (int i = 0; i < item->childCount(); ++i) {
			setRecursiveUniformValues(item->child(i), u);
		}
	}
}

typedef enum {
	FORCE_NONE,
	FORCE_ARRAY,
	FORCE_STRUCT,
	FORCE_MATRIX,
	FORCE_VECTOR
} forceTypes;

static void addShVariable(const ShVariable *i_pVar, QString i_qName,
		bool i_bForceBuiltIn, forceTypes i_eForceType, const int i_pSubs[],
		OldShVarItem *i_qParent)
{
	int i, j[2], id;
	QList<QVariant> varData;
	QString fullName;
	QString shortName;
	char *typeString = NULL;

	/* Add data known */
	switch (i_eForceType) {
	case FORCE_NONE:
	default:
		shortName = QString(i_pVar->name);
		fullName = QString(i_pVar->name);
		id = i_pVar->uniqueId;
		break;
	case FORCE_ARRAY:
		shortName = QString("[") + QVariant(i_pSubs[0]).toString()
				+ QString("]");
		fullName = i_qName + shortName;
		id = -1;
		break;
	case FORCE_STRUCT:
		shortName = QString(i_pVar->name);
		fullName = i_qName + QString(".") + shortName;
		id = -1;
		break;
	case FORCE_MATRIX:
		shortName = QString("[") + QVariant(i_pSubs[0]).toString()
				+ QString("]") + QString("[") + QVariant(i_pSubs[1]).toString()
				+ QString("]");
		fullName = i_qName + shortName;
		id = -1;
		break;
	case FORCE_VECTOR:
		switch (i_pSubs[0]) {
		case 0:
			shortName = QString("x");
			break;
		case 1:
			shortName = QString("y");
			break;
		case 2:
			shortName = QString("z");
			break;
		case 3:
			shortName = QString("w");
			break;

		}
		fullName = i_qName + QString(".") + shortName;
		id = -1;
		break;
	}

	typeString = ShGetTypeString(i_pVar);

	varData << shortName; /* DF_NAME */
	varData << fullName; /* DF_FULLNAME */
	varData << typeString; /* DF_TYPE */
	varData << i_pVar->type; /* DF_TYPEID */
	varData << ShGetQualifierString(i_pVar); /* DF_QUALIFIER */
	varData << id; /* DF_UNIQUE_ID */
	switch (i_eForceType) {
	case FORCE_ARRAY:
	case FORCE_VECTOR:
		varData << SH_CGB_ARRAY; /* DF_CGBL_TYPE */
		varData << i_pSubs[0]; /* DF_CGBL_INDEX_A */
		varData << QVariant(); /* DF_CGBL_INDEX_B */
		break;
	case FORCE_STRUCT:
		varData << SH_CGB_STRUCT; /* DF_CGBL_TYPE */
		varData << i_pSubs[0]; /* DF_CGBL_INDEX_A */
		varData << QVariant(); /* DF_CGBL_INDEX_B */
		break;
	case FORCE_MATRIX:
		varData << SH_CGB_ARRAY; /* DF_CGBL_TYPE */
		varData << i_pSubs[0]; /* DF_CGBL_INDEX_A */
		varData << i_pSubs[1]; /* DF_CGBL_INDEX_B */
		break;
	default:
		varData << QVariant(); /* DF_CGBL_TYPE */
		varData << QVariant(); /* DF_CGBL_INDEX_A */
		varData << QVariant(); /* DF_CGBL_INDEX_B */
	}
	if (i_bForceBuiltIn) {
		varData << true; /* DF_BUILTIN */
	} else {
		varData << bool(i_pVar->builtin); /* DF_BUILTIN */
	}
	varData << false; /* DF_CHANGED */
	if (i_bForceBuiltIn || i_pVar->builtin) {
		varData << OldShVarItem::InScope; /* DF_SCOPE */
	} else {
		varData << OldShVarItem::OutOfScope; /* DF_SCOPE */
	}

	// Note: Allow selecting all but samplers and batch insert into watch list.
	if (i_pVar->type == SH_SAMPLER) {
		varData << false; /* DF_SELECTABLE */
	} else {
		varData << true; /* DF_SELECTABLE */
	}
	varData << false; /* DF_WATCHED */
	varData << QVariant::fromValue<void*>(NULL); /* DF_DATA_PIXELBOX */
	varData << QVariant::fromValue<void*>(NULL); /* DF_DATA_VERTEXBOX */
	varData << QVariant::fromValue<void*>(NULL); /* DF_DATA_CURRENTBOX */
	varData << QVariant(); /* DF_DEBUG_SELECTED_VALUE */
	varData << QVariant(); /* DF_DEBUG_UNIFORM_VALUE */

	free(typeString);

	OldShVarItem *newVar = new OldShVarItem(varData, i_qParent);
	i_qParent->appendChild(newVar);

	if (i_pVar->isArray) {
		ShVariable *arrayAtom = copyShVariable((ShVariable*) i_pVar);

		/* delete first array */
		int i;
		for (i = 0; i < 2; i++) {
			if (i + 1 >= 2) {
				arrayAtom->arraySize[i] = -1;
			} else {
				arrayAtom->arraySize[i] = i_pVar->arraySize[i + 1];
			}
		}

		if (arrayAtom->arraySize[0] == -1) {
			arrayAtom->isArray = false;
		} else {
			arrayAtom->isArray = true;
		}

		for (i = 0; i < i_pVar->arraySize[0]; i++) {
			addShVariable(arrayAtom, fullName, i_pVar->builtin, FORCE_ARRAY, &i,
					newVar);
		}

		freeShVariable(&arrayAtom);
		return;
	}

	if (i_pVar->structSize) {
		for (i = 0; i < i_pVar->structSize; i++) {
			ShVariable *structElement = copyShVariable(
					(ShVariable*) i_pVar->structSpec[i]);
			structElement->qualifier = i_pVar->qualifier;
			structElement->varyingModifier = i_pVar->varyingModifier;
			addShVariable(structElement, fullName, i_pVar->builtin,
					FORCE_STRUCT, &i, newVar);
		}
		return;
	}

	if (i_pVar->isMatrix) {
		ShVariable *arrayAtom = copyShVariable((ShVariable*) i_pVar);
		arrayAtom->isMatrix = false;
		arrayAtom->size = 0;
		arrayAtom->matrixColumns = 0;

		for (j[0] = 0; j[0] < (i_pVar->size / i_pVar->matrixColumns); j[0]++) {
			for (j[1] = 0; j[1] < i_pVar->matrixColumns; j[1]++) {
				addShVariable(arrayAtom, fullName, i_bForceBuiltIn,
						FORCE_MATRIX, j, newVar);
			}
		}

		freeShVariable(&arrayAtom);
		return;
	}

	if (1 < i_pVar->size) {
		ShVariable *arrayAtom = copyShVariable((ShVariable*) i_pVar);
		arrayAtom->size = 1;
		for (i = 0; i < i_pVar->size; i++) {
			addShVariable(arrayAtom, fullName, i_bForceBuiltIn, FORCE_VECTOR,
					&i, newVar);
		}

		freeShVariable(&arrayAtom);
		return;
	}

}

void OldShVarModel::setRecursiveWatched(OldShVarItem *i_qItem)
{
	if (!i_qItem)
		return;

	i_qItem->setData(DF_WATCHED, true);
	emit dataChanged(OldShVarModel::getIndex(i_qItem, 0),
			OldShVarModel::getIndex(i_qItem, DF_LAST - 1));

	if (i_qItem->parent() != m_qRootItem) {
		setRecursiveWatched(i_qItem->parent());
	}
}

void OldShVarModel::setRecursiveExpand(OldShVarItem *i_qItem)
{
	int i;

	if (!i_qItem)
		return;

	if (i_qItem->parent() != m_qRootItem) {
		setRecursiveExpand(i_qItem->parent());
	}

	if (m_qWatchList) {
		for (i = 0; i < DF_LAST; i++) {
			QModelIndex testItem = OldShVarModel::getIndex(i_qItem, i);
			QModelIndex watchItem = m_qWatchProxy->mapFromSource(testItem);
			m_qWatchList->expand(watchItem);
		}
		emit dataChanged(OldShVarModel::getIndex(i_qItem, 0),
				OldShVarModel::getIndex(i_qItem, DF_LAST - 1));
	}
}

void OldShVarModel::setItemWatched(const QModelIndex & i_qIndex)
{
	if (i_qIndex.isValid()) {
		OldShVarItem *item = static_cast<OldShVarItem*>(i_qIndex.internalPointer());
		if ((item->data(DF_SELECTABLE).toBool() == true)
				&& (item->childCount() > 0
						|| item->data(DF_WATCHED).toBool() == false)) {

			if (item->childCount() < 1) {
				m_qWatchListItems << item;

				setRecursiveWatched(item);

				m_qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
				setRecursiveExpand(item);
				emit newWatchItem(item);

			} else {
				// Note: According to Thomas, each variable must be read
				// separately when adding, so batching the process will not give
				// much speedup compared to this recursion.
				for (int i = 0; i < item->childCount(); i++) {
					// TODO: There is possible a more elegant solution to get
					// indices of the children, but I am no Qt expert and this
					// seems to work somehow.
					this->setItemWatched(i_qIndex.child(i, i_qIndex.column()));
				}
			}
		}

		// Black magic: Watch list shows non-watched items if we do not reset
		// the filter here (Qt 4.3.2).
		m_qWatchProxy->setFilterRegExp(QRegExp("(true|1)"));
	}
}

void OldShVarModel::unsetRecursiveWatched(OldShVarItem *i_qItem)
{
	bool descent = true;
	int i;
	if (!i_qItem)
		return;

	for (i = 0; i < i_qItem->childCount(); i++) {
		if (i_qItem->child(i)->data(DF_WATCHED) == true) {
			descent = false;
		}
	}

	if (descent) {
		i_qItem->setData(DF_WATCHED, false);

		if (i_qItem->parent() != m_qRootItem) {
			unsetRecursiveWatched(i_qItem->parent());
		}
	}
}

void OldShVarModel::unsetItemWatched(const QModelIndex & i_qIndex)
{
	QStack<QModelIndex> stack;
	OldShVarItem *item = NULL;

	if (i_qIndex.isValid()) {
		stack.push(m_qWatchProxy->mapToSource(i_qIndex));

		while (!stack.isEmpty()) {
			QModelIndex idx = stack.pop();
			item = static_cast<OldShVarItem*>(idx.internalPointer());

			if (item != NULL) {
				item->setData(DF_WATCHED, false);
				if (item->childCount() > 0) {
					/* Item is root or inner node. */
					for (int i = 0; i < item->childCount(); i++) {
						// TODO: There is possible a more elegant solution to
						// get indices of the children, but I am no Qt expert
						// and this seems to work somehow.
						stack.push(idx.child(i, idx.column()));
					}
				} else {
					/* Item is leaf node. */
					if (item->getPixelBoxPointer() != NULL) {
						PixelBox *fb = item->getPixelBoxPointer();
						item->setPixelBoxPointer(NULL);
						delete fb;
					}
					if (item->getCurrentPointer() != NULL) {
						VertexBox *vb = item->getCurrentPointer();
						item->setCurrentPointer(NULL);
						delete vb;
					}
					if (item->getVertexBoxPointer() != NULL) {
						VertexBox *vb = item->getVertexBoxPointer();
						item->setVertexBoxPointer(NULL);
						delete vb;
					} /* end if (item->childCount() > 0) */

					m_qWatchListItems.removeAll(item);
				} /* end if (item->childCount() > 0) */
			} /* if (item != NULL) */
		} /* end while (!stack.isEmpty()) */
		/* 'item' should now be the item associated with 'i_qIndex'. */

		/*
		 * Remove parent recursively when all children have left the watch list.
		 * This must not be done for child nodes of 'item' as (i) these are
		 * guaranteed to be removed be the iterative tree traversal above and
		 * (ii) too many nodes will be removed if doing so because because the
		 * tree is reorganised as nodes are removed and the indices of removed
		 * nodes are reused under certain circumstances.
		 */
		if (item != NULL) {
			unsetRecursiveWatched(item);
		}

		/* used to update the whole tree,
		 * it seems just emiting the signal is not enough for qt 4.2.2 */
		m_qWatchProxy->setFilterRegExp(QRegExp("(true|1)"));

	} /* end if (i_qIndex.isValid()) */
}

OldShVarItem* OldShVarModel::getWatchItemPointer(const QModelIndex & i_qIndex)
{
	if (i_qIndex.isValid()) {
		QModelIndex qSourceIndex = m_qWatchProxy->mapToSource(i_qIndex);
		OldShVarItem *item =
				static_cast<OldShVarItem*>(qSourceIndex.internalPointer());
		return item;
	} else {
		return NULL;
	}
}

QList<OldShVarItem*> OldShVarModel::getAllWatchItemPointers(void)
{
	return m_qWatchListItems;
}

QList<OldShVarItem*> OldShVarModel::getChangedWatchItemPointers(void)
{
	QList<OldShVarItem*> changedWatchItems;
	int i;

	for (i = 0; i < m_qWatchListItems.count(); i++) {
		OldShVarItem* item = m_qWatchListItems[i];
		if (item->data(DF_CHANGED).toBool() == true) {
			changedWatchItems << item;
		}
	}

	return changedWatchItems;
}

QSortFilterProxyModel* OldShVarModel::getFilterModel(tvDisplayRole role)
{
	switch (role) {
	case TV_BUILTIN:
		return m_qBuiltInProxy;
	case TV_SCOPE:
		return m_qScopeProxy;
	case TV_WATCH_LIST:
		return m_qWatchProxy;
	case TV_UNIFORM:
		return m_qUniformProxy;
	default:
		return NULL;
	}
}

void OldShVarModel::onDoubleClickedAll(const QModelIndex & i_qIndex)
{
	if (i_qIndex.isValid()) {
		QModelIndex qSourceIndex = m_qAllProxy->mapToSource(i_qIndex);
		setItemWatched(qSourceIndex);
	}
}

void OldShVarModel::onDoubleClickedBuiltIns(const QModelIndex & i_qIndex)
{
	if (i_qIndex.isValid()) {
		QModelIndex qSourceIndex = m_qBuiltInProxy->mapToSource(i_qIndex);
		setItemWatched(qSourceIndex);
	}
}

void OldShVarModel::onDoubleClickedScope(const QModelIndex & i_qIndex)
{
	if (i_qIndex.isValid()) {
		QModelIndex qSourceIndex = m_qScopeProxy->mapToSource(i_qIndex);
		setItemWatched(qSourceIndex);
	}
}

void OldShVarModel::onDoubleClickedUniforms(const QModelIndex & i_qIndex)
{
	if (i_qIndex.isValid()) {
		QModelIndex qSourceIndex = m_qUniformProxy->mapToSource(i_qIndex);
		setItemWatched(qSourceIndex);
	}
}

void OldShVarModel::setupModelData(const ShVariableList *i_pVL,
		OldShVarItem *i_qParent)
{
	int i;

	for (i = 0; i < i_pVL->numVariables; i++) {
		addShVariable(i_pVL->variables[i], QString(), false, FORCE_NONE, NULL,
				i_qParent);
	}
}

void OldShVarModel::attach(QTreeView *tView, tvDisplayRole role)
{
	switch (role) {
	case TV_ALL:
		tView->setModel(m_qAllProxy);
		tView->setColumnHidden(DF_FULLNAME, true);
		tView->setColumnHidden(DF_TYPEID, true);
		tView->setColumnHidden(DF_UNIQUE_ID, true);
		tView->setColumnHidden(DF_BUILTIN, true);
		tView->setColumnHidden(DF_CHANGED, true);
		tView->setColumnHidden(DF_SCOPE, true);
		tView->setColumnHidden(DF_SELECTABLE, true);
		tView->setColumnHidden(DF_WATCHED, true);
		tView->setColumnHidden(DF_CGBL_TYPE, true);
		tView->setColumnHidden(DF_CGBL_INDEX_A, true);
		tView->setColumnHidden(DF_CGBL_INDEX_B, true);
		tView->setColumnHidden(DF_DATA_PIXELBOX, true);
		tView->setColumnHidden(DF_DATA_VERTEXBOX, true);
		tView->setColumnHidden(DF_DATA_CURRENTBOX, true);
		tView->setColumnHidden(DF_DEBUG_SELECTED_VALUE, true);
		tView->setColumnHidden(DF_DEBUG_UNIFORM_VALUE, true);
		/* Only allow variables from actual scope to be added to watchlist
		 connect(tView, SIGNAL(doubleClicked(const QModelIndex &)),
		 this, SLOT(onDoubleClickedAll(const QModelIndex &)));
		 */
		break;
	case TV_BUILTIN:
		tView->setModel(m_qBuiltInProxy);
		tView->setColumnHidden(DF_FULLNAME, true);
		tView->setColumnHidden(DF_TYPEID, true);
		tView->setColumnHidden(DF_UNIQUE_ID, true);
		tView->setColumnHidden(DF_BUILTIN, true);
		tView->setColumnHidden(DF_CHANGED, true);
		tView->setColumnHidden(DF_SCOPE, true);
		tView->setColumnHidden(DF_SELECTABLE, true);
		tView->setColumnHidden(DF_WATCHED, true);
		tView->setColumnHidden(DF_CGBL_TYPE, true);
		tView->setColumnHidden(DF_CGBL_INDEX_A, true);
		tView->setColumnHidden(DF_CGBL_INDEX_B, true);
		tView->setColumnHidden(DF_DATA_PIXELBOX, true);
		tView->setColumnHidden(DF_DATA_VERTEXBOX, true);
		tView->setColumnHidden(DF_DATA_CURRENTBOX, true);
		tView->setColumnHidden(DF_DEBUG_SELECTED_VALUE, true);
		tView->setColumnHidden(DF_DEBUG_UNIFORM_VALUE, true);
		connect(tView, SIGNAL(doubleClicked(const QModelIndex &)), this,
				SLOT(onDoubleClickedBuiltIns(const QModelIndex &)));
		break;
	case TV_SCOPE:
		tView->setModel(m_qScopeProxy);
		tView->setColumnHidden(DF_FULLNAME, true);
		tView->setColumnHidden(DF_TYPEID, true);
		tView->setColumnHidden(DF_UNIQUE_ID, true);
		tView->setColumnHidden(DF_BUILTIN, true);
		tView->setColumnHidden(DF_CHANGED, true);
		tView->setColumnHidden(DF_SCOPE, true);
		tView->setColumnHidden(DF_SELECTABLE, true);
		tView->setColumnHidden(DF_WATCHED, true);
		tView->setColumnHidden(DF_CGBL_TYPE, true);
		tView->setColumnHidden(DF_CGBL_INDEX_A, true);
		tView->setColumnHidden(DF_CGBL_INDEX_B, true);
		tView->setColumnHidden(DF_DATA_PIXELBOX, true);
		tView->setColumnHidden(DF_DATA_VERTEXBOX, true);
		tView->setColumnHidden(DF_DATA_CURRENTBOX, true);
		tView->setColumnHidden(DF_DEBUG_SELECTED_VALUE, true);
		tView->setColumnHidden(DF_DEBUG_UNIFORM_VALUE, true);
		connect(tView, SIGNAL(doubleClicked(const QModelIndex &)), this,
				SLOT(onDoubleClickedScope(const QModelIndex &)));
		break;
	case TV_UNIFORM:
		tView->setModel(m_qUniformProxy);
		tView->setColumnHidden(DF_FULLNAME, true);
		tView->setColumnHidden(DF_TYPEID, true);
		tView->setColumnHidden(DF_QUALIFIER, true);
		tView->setColumnHidden(DF_UNIQUE_ID, true);
		tView->setColumnHidden(DF_BUILTIN, true);
		tView->setColumnHidden(DF_CHANGED, true);
		tView->setColumnHidden(DF_SCOPE, true);
		tView->setColumnHidden(DF_SELECTABLE, true);
		tView->setColumnHidden(DF_WATCHED, true);
		tView->setColumnHidden(DF_CGBL_TYPE, true);
		tView->setColumnHidden(DF_CGBL_INDEX_A, true);
		tView->setColumnHidden(DF_CGBL_INDEX_B, true);
		tView->setColumnHidden(DF_DATA_PIXELBOX, true);
		tView->setColumnHidden(DF_DATA_VERTEXBOX, true);
		tView->setColumnHidden(DF_DATA_CURRENTBOX, true);
		tView->setColumnHidden(DF_DEBUG_SELECTED_VALUE, true);
		connect(tView, SIGNAL(doubleClicked(const QModelIndex &)), this,
				SLOT(onDoubleClickedUniforms(const QModelIndex &)));
		break;
	case TV_WATCH_LIST:
		tView->setModel(m_qWatchProxy);
		tView->setColumnHidden(DF_FULLNAME, true);
		tView->setColumnHidden(DF_TYPEID, true);
		tView->setColumnHidden(DF_UNIQUE_ID, true);
		tView->setColumnHidden(DF_BUILTIN, true);
		tView->setColumnHidden(DF_CHANGED, true);
		tView->setColumnHidden(DF_SCOPE, true);
		tView->setColumnHidden(DF_SELECTABLE, true);
		tView->setColumnHidden(DF_WATCHED, true);
		tView->setColumnHidden(DF_CGBL_TYPE, true);
		tView->setColumnHidden(DF_CGBL_INDEX_A, true);
		tView->setColumnHidden(DF_CGBL_INDEX_B, true);
		tView->setColumnHidden(DF_DATA_PIXELBOX, true);
		tView->setColumnHidden(DF_DATA_VERTEXBOX, true);
		tView->setColumnHidden(DF_DATA_CURRENTBOX, true);
		tView->setColumnHidden(DF_DEBUG_UNIFORM_VALUE, true);
		tView->setSelectionBehavior(QAbstractItemView::SelectRows);
		tView->setSelectionMode(QAbstractItemView::ExtendedSelection);
		m_qWatchList = tView;
		break;
	default:
		break;
	}
}

void OldShVarModel::detach(QAbstractItemView *view)
{
	view->setModel(NULL);

	if (view == m_qWatchList) {
		m_qWatchList = NULL;
	}
}

ScopeSortFilterProxyModel::ScopeSortFilterProxyModel(QObject *parent) :
		QSortFilterProxyModel(parent)
{
}

bool ScopeSortFilterProxyModel::filterAcceptsRow(int sourceRow,
		const QModelIndex &sourceParent) const
{
	QModelIndex scopeIndex = sourceModel()->index(sourceRow, DF_SCOPE,
			sourceParent);
	QModelIndex builtInIndex = sourceModel()->index(sourceRow, DF_BUILTIN,
			sourceParent);
	QModelIndex typeIndex = sourceModel()->index(sourceRow, DF_TYPEID,
			sourceParent);
	QModelIndex qualifierIndex = sourceModel()->index(sourceRow, DF_QUALIFIER,
			sourceParent);

	// no builtins, no uniforms, but everything else in scope
	return !sourceModel()->data(builtInIndex).toBool()
			&& sourceModel()->data(qualifierIndex).toString() != "uniform"
			&& (sourceModel()->data(scopeIndex).toInt() == OldShVarItem::InScope
					|| sourceModel()->data(scopeIndex).toInt()
							== OldShVarItem::NewInScope
					|| sourceModel()->data(scopeIndex).toInt()
							== OldShVarItem::InScopeStack);
}

SamplerSortFilterProxyModel::SamplerSortFilterProxyModel(QObject *parent) :
		QSortFilterProxyModel(parent)
{
}

bool SamplerSortFilterProxyModel::filterAcceptsRow(int sourceRow,
		const QModelIndex &sourceParent) const
{
	QModelIndex typeIndex = sourceModel()->index(sourceRow, DF_TYPEID,
			sourceParent);

	// only samplers
	return ((variableType) sourceModel()->data(typeIndex).toInt()) == SH_SAMPLER;
}

UniformSortFilterProxyModel::UniformSortFilterProxyModel(QObject *parent) :
		QSortFilterProxyModel(parent)
{
}

bool UniformSortFilterProxyModel::filterAcceptsRow(int sourceRow,
		const QModelIndex &sourceParent) const
{
	QModelIndex typeIndex = sourceModel()->index(sourceRow, DF_TYPEID,
			sourceParent);
	QModelIndex qualifierIndex = sourceModel()->index(sourceRow, DF_QUALIFIER,
			sourceParent);
	QModelIndex builtInIndex = sourceModel()->index(sourceRow, DF_BUILTIN,
			sourceParent);

	// only user declared uniforms, no built-ins
	return sourceModel()->data(qualifierIndex).toString() == "uniform"
			&& !sourceModel()->data(builtInIndex).toBool();
}

