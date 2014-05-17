
#include "geometrydatamodel.h"
#include "geometrytreeitem.h"
#include "data/vertexBox.h"
#include "utils/dbgprint.h"
#include <QtOpenGL>

#define DATA_STRIDE 3

GeometryDataSortFilterProxyModel::GeometryDataSortFilterProxyModel(
		QObject *parent) :
		QSortFilterProxyModel(parent)
{
	hideInactive = false;
	hideEmpty = false;
}

void GeometryDataSortFilterProxyModel::setHideInactive(bool b)
{
	hideInactive = b;
	reset();
}

void GeometryDataSortFilterProxyModel::setHideEmpty(bool b)
{
	hideEmpty = b;
	reset();
}

bool GeometryDataSortFilterProxyModel::filterAcceptsRow(int sourceRow,
		const QModelIndex &sourceParent) const
{
	bool accept = true;
	GeometryDataModel *source = dynamic_cast<GeometryDataModel*>(sourceModel());
	QModelIndex index = source->index(sourceRow, 0, sourceParent);
	if (hideInactive && source->flags(index) == 0)
		accept = false;
	if (hideEmpty && source->noOutputPrims(index))
		accept = false;
	return accept;
}

bool GeometryDataModel::isBasicPrimitive(int primType)
{
	switch (primType) {
	case GL_POINTS:
	case GL_LINES:
	case GL_LINE_STRIP:
	case GL_LINE_LOOP:
	case GL_LINES_ADJACENCY_EXT:
	case GL_LINE_STRIP_ADJACENCY_EXT:
	case GL_TRIANGLES:
	case GL_TRIANGLE_STRIP:
	case GL_TRIANGLE_FAN:
	case GL_TRIANGLE_MESH_SUN:
	case GL_TRIANGLES_ADJACENCY_EXT:
	case GL_TRIANGLE_STRIP_ADJACENCY_EXT:
		return true;
	case GL_QUADS:
	case GL_QUAD_STRIP:
	case GL_POLYGON:
	case GL_QUAD_MESH_SUN:
		return false;
	default:
		fprintf(stderr, "E! Unsupported primitive type\n");
		return false;
	}
}

int GeometryDataModel::generateVerticles(const float *data, int offset, int count,
										  int total, GeometryTreeItem *primitive)
{
	if (count <= 0)
		return 0;

	int generated = 0;
	const float *primMap = data + offset * DATA_STRIDE;
	int primitiveIdOut = primMap[0];
	int maxIndex = primMap[1] - 1;
	int numVertices = 0;
	int k = 0;
	dbgPrint(DBGLVL_INFO, "\t\tOUT PRIM %i\n", k);
	GeometryTreeItem *outPrimItem = NULL;
	if (!condition) {
		outPrimItem = new GeometryTreeOutPrimitive(
					QString("OUT PRIM ") + QString::number(k));
		primitive->appendRow(outPrimItem);
	}
	numOutPrimitives++;

	do {
		int map_index = primMap[1]; // -1 ?
		if (maxIndex < map_index) {
			dbgPrint(DBGLVL_INFO, "\t\t\tVERTEX %i\n", numVertices);
			if (!condition) {
				outPrimItem->appendRow(new GeometryTreeVertex(
							QString("VERTEX ") + QString::number(numVertices),
							offset, &vertexData));
			}
			numOutVertices++;
			numVertices++;
			maxIndex = map_index;
		}
		generated++;
		primMap += DATA_STRIDE;

		int map_id = primMap[0];
		if (numVertices < count && map_id != primitiveIdOut) {
			k++;
			primitiveIdOut = map_id;
			maxIndex = primMap[1] - 1;
			dbgPrint(DBGLVL_INFO, "\t\tOUT PRIM1 %i\n", k);
			if (!condition) {
				outPrimItem = new GeometryTreeOutPrimitive(
							QString("OUT PRIM ") + QString::number(k));
				primitive->appendRow(outPrimItem);
			}
			numOutPrimitives++;
		}
	} while (numVertices < count && generated < total);

	return generated;
}

GeometryDataModel::GeometryDataModel(int inPrimitiveType,
		int outPrimitiveType, VertexBox *primitiveMap,
		VertexBox *vertexCountMap, VertexBox *_condition, bool *initialCoverage,
		QObject *parent) :
		QAbstractItemModel(parent)
{
	inPrimitive = inPrimitiveType;
	outPrimitive = outPrimitiveType;
	numSubInPrimitives = 0;
	numOutPrimitives = 0;
	numOutVertices = 0;
	numInPrimitives = 0;
	condition = _condition;
	coverage = initialCoverage;
	rootItem = new GeometryTreeItem("ROOT", -1, &currentData);

	const float *vcMap = vertexCountMap->getDataPointer();
	const float *primMap = primitiveMap->getDataPointer();

	int generated = 0;
	int numVertices = primitiveMap->getNumVertices();
	int inPrimitives = vertexCountMap->getNumVertices();

	// FIXME: WTF IS THIS? So much loops made me cry.
	//        This must be rewritten somehow
	bool basic = isBasicPrimitive(inPrimitiveType);
	QList<VertexBox*>* cdata = basic ? &currentData : NULL;
	for (int i = 0; i < inPrimitives;) {
		int primitiveIdIn = vcMap[DATA_STRIDE * i + 2];
		dbgPrint(DBGLVL_INFO, "INPUT PRIM %i\n", primitiveIdIn);
		GeometryTreeItem *inPrimItem = new GeometryTreeInPrimitive(
					QString("IN PRIM") + QString::number(primitiveIdIn),
					basic ? i : -1, cdata, condition, coverage);
		numInPrimitives++;
		rootItem->appendRow(inPrimItem);
		GeometryTreeItem *currentInPrim = inPrimItem;

		do {
			int vertexCount = vcMap[DATA_STRIDE * i];
			//int numVerticesNotConsumed = vcMap[DATA_STRIDE * i + 1];

			if (!basic) {
				dbgPrint(DBGLVL_INFO, "\tSUB PRIM %i(%i)\n", primitiveIdIn, numSubInPrimitives);
				currentInPrim = new GeometryTreeInPrimitive(
							QString("SUB PRIM") + QString::number(numSubInPrimitives),
							numSubInPrimitives, &currentData, condition, coverage);
				inPrimItem->appendRow(currentInPrim);
				numSubInPrimitives++;
			}

			generated += generateVerticles(primMap, generated, vertexCount,
										   numVertices - generated, currentInPrim);
		} while (++i < inPrimitives && primitiveIdIn == vcMap[DATA_STRIDE * i + 2]);
	}
}

GeometryDataModel::~GeometryDataModel()
{
	currentData.clear();
	vertexData.clear();
	dataNames.clear();
	delete rootItem;
}

bool GeometryDataModel::addData(VertexBox *current, VertexBox* vertex,
		QString &name)
{
	if (currentData.contains(current))
		return false;

	currentData.append(current);
	vertexData.append(vertex);
	dataNames.append(name);
	connect(current, SIGNAL(dataDeleted()), this, SLOT(detachData()));
	connect(current, SIGNAL(dataChanged()), this, SLOT(updateData()));
	updateData();
	return true;
}

#define STRINGFY_CASE(val) case val: return #val;

QString GeometryDataModel::primitiveString(int type)
{
	switch (type) {
	STRINGFY_CASE(GL_POINTS)
	STRINGFY_CASE(GL_LINES)
	STRINGFY_CASE(GL_LINE_STRIP)
	STRINGFY_CASE(GL_LINE_LOOP)
	STRINGFY_CASE(GL_LINES_ADJACENCY_EXT)
	STRINGFY_CASE(GL_LINE_STRIP_ADJACENCY_EXT)
	STRINGFY_CASE(GL_TRIANGLES)
	STRINGFY_CASE(GL_QUADS)
	STRINGFY_CASE(GL_QUAD_STRIP)
	STRINGFY_CASE(GL_TRIANGLE_STRIP)
	STRINGFY_CASE(GL_TRIANGLE_FAN)
	STRINGFY_CASE(GL_POLYGON)
	STRINGFY_CASE(GL_QUAD_MESH_SUN)
	STRINGFY_CASE(GL_TRIANGLE_MESH_SUN)
	STRINGFY_CASE(GL_TRIANGLES_ADJACENCY_EXT)
	STRINGFY_CASE(GL_TRIANGLE_STRIP_ADJACENCY_EXT)
	default:
		return "GL_INVALID_VALUE";
	}
}
#undef STRINGFY_CASE


Qt::ItemFlags GeometryDataModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
	GeometryTreeItem *item = static_cast<GeometryTreeItem*>(index.internalPointer());
	return item->flags(index.column());
}

QModelIndex GeometryDataModel::index(int row, int column, const QModelIndex &parent) const
{
	GeometryTreeItem *parentItem;
	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<GeometryTreeItem*>(parent.internalPointer());

	GeometryTreeItem *childItem = dynamic_cast<GeometryTreeItem*>(parentItem->child(row));
	if (childItem)
		return createIndex(row, column, childItem);
	return QModelIndex();
}

QModelIndex GeometryDataModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();

	GeometryTreeItem *childItem = static_cast<GeometryTreeItem*>(index.internalPointer());
	GeometryTreeItem *parentItem = childItem->parent();
	if (parentItem == rootItem)
		return QModelIndex();
	return createIndex(parentItem->row(), 0, parentItem);
}

int GeometryDataModel::rowCount(const QModelIndex &parent) const
{
	GeometryTreeItem *parentItem;
	if (!parent.isValid())
		parentItem = rootItem;
	else
		parentItem = static_cast<GeometryTreeItem*>(parent.internalPointer());
	return parentItem->rowCount();
}

int GeometryDataModel::columnCount(const QModelIndex &parent) const
{
	if (parent.isValid())
		return static_cast<GeometryTreeItem*>(parent.internalPointer())->columnCount();
	return rootItem->columnCount() + (condition ? 1 : 0);
}

QVariant GeometryDataModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	GeometryTreeItem *item = static_cast<GeometryTreeItem*>(index.internalPointer());
	switch(role) {
	case Qt::DisplayRole:
		return item->data(index.column());
	case Qt::TextColorRole:
		return item->displayColor(index.column());
	case VertexRole:
		return item->isVertexItem();
	case IndexRole:
		return item->getDataIndex();
	default:
		return QVariant();
	}
}

bool GeometryDataModel::noOutputPrims(const QModelIndex &index) const
{
	if (index.isValid()) {
		GeometryTreeItem *item = static_cast<GeometryTreeItem*>(index.internalPointer());
		/* find input prim item */
		while (item->parent() != rootItem)
			item = item->parent();

		int rows = item->rowCount();
		if (item->getDataIndex() < 0) {
			/* non-basic input primitive -> check children */
			for (int i = 0; i < rows; i++) {
				if (item->child(i)->rowCount() != 0)
					return false;
			}
			return true;
		}
		return rows == 0;
	}
	return false;
}

int GeometryDataModel::getBasePrimitive(int type)
{
	switch (type) {
	case GL_POINTS:
		return GL_POINTS;
	case GL_LINES:
	case GL_LINE_STRIP:
	case GL_LINE_LOOP:
		return GL_LINES;
	case GL_LINES_ADJACENCY_EXT:
	case GL_LINE_STRIP_ADJACENCY_EXT:
		return GL_LINES_ADJACENCY_EXT;
	case GL_TRIANGLES:
	case GL_QUADS:
	case GL_QUAD_STRIP:
	case GL_TRIANGLE_STRIP:
	case GL_TRIANGLE_FAN:
	case GL_POLYGON:
	case GL_QUAD_MESH_SUN:
	case GL_TRIANGLE_MESH_SUN:
		return GL_TRIANGLES;
	case GL_TRIANGLES_ADJACENCY_EXT:
	case GL_TRIANGLE_STRIP_ADJACENCY_EXT:
		return GL_TRIANGLES_ADJACENCY_EXT;
	default:
		return GL_INVALID_VALUE;
	}
}

QVariant GeometryDataModel::headerData(int section,
		Qt::Orientation orientation, int role) const
{
	if (role == Qt::DisplayRole && orientation == Qt::Horizontal) {
		if (section == 0)
			return "Geometry Structure";

		if (condition) {
			if (section == 1)
				return "Condition";
			return dataNames[section - 2];
		}
		return dataNames[section - 1];
	}
	return QVariant();
}

void GeometryDataModel::detachData()
{
	VertexBox *vb = static_cast<VertexBox*>(sender());
	if (!vb)
		return;

	int idx = currentData.indexOf(vb);
	if (idx >= 0) {
		currentData.removeAt(idx);
		vertexData.removeAt(idx);
		dataNames.removeAt(idx);
		updateData();
	}

	emit dataDeleted(idx);
	if (currentData.isEmpty())
		emit empty();
}

void GeometryDataModel::updateData()
{
	QModelIndex indexBegin = index(0, 0);
	QModelIndex indexEnd = index(columnCount() - 1, rowCount() - 1);
	if (indexBegin.isValid() && indexEnd.isValid()) {
		emit dataChanged(indexBegin, indexEnd);
		emit layoutChanged();
	} else {
		reset();
	}
}

int GeometryDataModel::getNumInPrimitives() const
{
	return numInPrimitives;
}

int GeometryDataModel::getNumSubInPrimitives() const
{
	return numSubInPrimitives;
}

int GeometryDataModel::getNumOutPrimitives() const
{
	return numOutPrimitives;
}

int GeometryDataModel::getNumOutVertices() const
{
	return numOutVertices;
}

const QString& GeometryDataModel::getDataColumnName(int column) const
{
	static const QString es("");
	if (column >= 0 && column < dataNames.size())
		return dataNames[column];
	return es;
}

VertexBox* GeometryDataModel::getDataColumnVertexData(int column)
{
	if (column >= 0 && column < vertexData.size())
		return vertexData[column];
	return NULL;
}

VertexBox* GeometryDataModel::getDataColumnCurrentData(int column)
{
	if (column >= 0 && column < currentData.size())
		return currentData[column];
	return NULL;
}

int GeometryDataModel::getDataColumnCount() const
{
	return currentData.size();
}

