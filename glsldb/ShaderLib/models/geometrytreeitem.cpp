
#include "geometrytreeitem.h"
#include "data/vertexBox.h"
#include "colors.qt.h"

GeometryTreeItem::GeometryTreeItem(QString name, int idx, QList<VertexBox*> *data)
{
	itemName = name;
	dataIdx = idx;
	dataList = data;
}

GeometryTreeItem::~GeometryTreeItem()
{
}

int GeometryTreeItem::columnCount() const
{
	if (dataList)
		return dataList->count() + 1;
	return this->parent()->columnCount();
}

int GeometryTreeItem::getDataIndex() const
{
	return dataIdx;
}

QVariant GeometryTreeItem::data(int) const
{
	return QVariant();
}

QVariant GeometryTreeItem::displayColor(int) const
{
	return QVariant();
}

GeometryTreeItem *GeometryTreeItem::parent() const
{
	return dynamic_cast<GeometryTreeItem *>(QStandardItem::parent());
}

Qt::ItemFlags GeometryTreeItem::flags(int) const
{
	return 0;
}


GeometryTreeInPrimitive::GeometryTreeInPrimitive(QString name, int idx,
		QList<VertexBox*> *data, VertexBox *cond, const bool *initial) :
		GeometryTreeItem(name, idx, data)
{
	condition = cond;
	coverage = initial;
}

int GeometryTreeInPrimitive::columnCount() const
{
	if (dataList) {
		if (condition)
			return dataList->count() + 2;
		return dataList->count() + 1;
	} else if (condition) {
		return 2;
	}
	return 1;
}

QVariant GeometryTreeInPrimitive::data(int column) const
{
	if (column < 0)
		return QVariant();

	if (!column)
		return itemName;


	VertexBox* data = NULL;
	if (condition) {
		float dval = condition->getDataValue(dataIdx);
		if (coverage && column == 1) {
			if (coverage[dataIdx]) {
				if (condition->getCoverageValue(dataIdx)) {
					if (dval > 0.75f)
						return QString("active");
					return QString("done");
				}
				return QString("out");
			}
			return QVariant("%");
		} else if (flags(0) == 0) {
			return QVariant("%");
		} else if (column == 1) {
			if (dval > 0.75f)
				return QString("true");
			else if (dval > 0.25f)
				return QString("false");
			return QVariant("false");
		} else if (dataList && column - 2 >= 0 && column - 2 < dataList->size()) {
			data = dataList->at(column - 2);
		}
	} else {
		if (dataList && column - 1 >= 0 && column - 1 < dataList->size()) {
			data = dataList->at(column - 1);
			if (flags(0) == 0)
				return QVariant("%");			
		}
	}

	if (data) {
		if (!data->getDataMapValue(dataIdx))
			return QVariant("-");
		return QVariant(data->getDataValue(dataIdx));
	}

	return QVariant();
}

QVariant GeometryTreeInPrimitive::displayColor(int) const
{
	if (condition) {
		float dval = condition->getDataValue(dataIdx);
		if (coverage) {
			if (coverage[dataIdx]) {
				if (condition->getCoverageValue(dataIdx)) {
					if (dval > 0.75f)
						return DBG_GREEN;
					return DBG_RED;
				}
				return DBG_ORANGE;
			}
		} else {
			if (dval > 0.75f)
				return DBG_GREEN;
			else if (dval > 0.25f)
				return DBG_RED;
		}
	}
	return QVariant();
}

Qt::ItemFlags GeometryTreeInPrimitive::flags(int column) const
{
	if (!column) {
		VertexBox *vb = NULL;
		if (condition)
			vb = condition;
		else if (dataList && !dataList->empty())
			vb = dataList->at(0);

		if (vb && vb->getCoveragePointer() && !vb->getCoveragePointer()[dataIdx])
			return 0;

		if (!dataList) {
			/* non-basic input primitive */
			for (int i = 0; i < rowCount(); i++) {
				GeometryTreeItem* c = dynamic_cast<GeometryTreeItem *>(child(i));
				Qt::ItemFlags cflags = c->flags(column);
				if (cflags != 0)
					return cflags;
			}
			return 0;
		}
	}

	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

GeometryTreeOutPrimitive::GeometryTreeOutPrimitive(QString name) :
		GeometryTreeItem(name, -1, NULL)
{
}

QVariant GeometryTreeOutPrimitive::data(int column) const
{
	if (!column)
		return itemName;
	return QVariant();
}

Qt::ItemFlags GeometryTreeOutPrimitive::flags(int column) const
{
	if (!column && !dataList && parent())
		return parent()->flags(0);
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

GeometryTreeVertex::GeometryTreeVertex(QString name, int idx,
		QList<VertexBox*> *data) :
		GeometryTreeItem(name, idx, data)
{
}

QVariant GeometryTreeVertex::data(int column) const
{
	if (column < 0)
		return QVariant();

	if (!column)
		return itemName;

	int list_idx = column - 1;
	if (dataList && list_idx < dataList->size()) {
		VertexBox* vb = dataList->at(list_idx);
		float dpval = vb->getDataValue(2 * dataIdx + 1);
		if (!vb->getDataMapValue(dataIdx)) {
			return QVariant("-");
		} else if (dpval == 0.0f || dpval == -0.0f) {
			return QVariant(QString("?"));
		} else if (dpval < 0.0) {
			return QVariant("-");
		} else if (dpval > 0.0) {
			if (flags(0) == 0)
				return QVariant("%");
			return QVariant(vb->getDataValue(2 * dataIdx));
		}
	}

	return QVariant();
}

Qt::ItemFlags GeometryTreeVertex::flags(int column) const
{
	return parent()->parent()->flags(column);
}
