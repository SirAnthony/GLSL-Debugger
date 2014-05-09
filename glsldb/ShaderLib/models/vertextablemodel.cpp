
#include "colors.qt.h"
#include "vertextablemodel.h"

VertexTableSortFilterProxyModel::VertexTableSortFilterProxyModel(QObject *parent) :
		QSortFilterProxyModel(parent)
{
	hideInactive = false;
}

void VertexTableSortFilterProxyModel::setHideInactive(bool b)
{
	hideInactive = b;
	reset();
}

bool VertexTableSortFilterProxyModel::filterAcceptsRow(int row,
								const QModelIndex &parent) const
{
	QAbstractItemModel *source = sourceModel();
	QModelIndex index = source->index(row, 0, parent);
	if (hideInactive)
		return source->flags(index) != 0;
	return true;
}




VertexTableModel::VertexTableModel(QObject *parent) :
		QAbstractTableModel(parent)
{
	boxCondition = NULL;
	coverage = NULL;
}

VertexTableModel::~VertexTableModel()
{
	boxData.clear();
	names.clear();
}

bool VertexTableModel::addVertexBox(VertexBox *vb, QString &name)
{
	foreach (VertexBox* data, boxData)
		if (data == vb)
			return false;
	boxData.append(vb);
	names.append(name);
	connect(vb, SIGNAL(dataDeleted()), this, SLOT(detachData()));
	connect(vb, SIGNAL(dataChanged()), this, SLOT(updateData()));
	reset();
	return true;
}

void VertexTableModel::setCondition(VertexBox *condition, bool *initialCoverage)
{
	boxCondition = condition;
	coverage = initialCoverage;
}

int VertexTableModel::rowCount(const QModelIndex &) const
{
	if (boxCondition)
		return boxCondition->getNumVertices();
	else if (!boxData.isEmpty())
		return boxData[0]->getNumVertices();
	return 0;
}

int VertexTableModel::columnCount(const QModelIndex &) const
{
	if (boxCondition)
		return boxData.size() + 1;
	return boxData.size();
}

QVariant VertexTableModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	int row = index.row();
	int column = index.column();

	if (boxCondition) {
		if (column >= boxData.size() + 1 || row >= boxCondition->getNumVertices())
			return QVariant();

		bool box_coverage = boxCondition->getCoverageValue(row);
		double box_data = boxCondition->getDataValue(row);
		if (role == Qt::DisplayRole) {
			if (coverage && column == 0) {
				if (coverage[row]) {
					if (box_coverage) {
						if (box_data > 0.75f)
							return QString("active");
						return QString("done");
					}
					return QString("out");
				}
				return QVariant("%");
			} else if (box_coverage) {
				if (column == 0) {
					if (box_data > 0.75f)
						return QString("true");
					return QVariant("false");
				} else {
					if (!boxData[column - 1]->getDataMapValue(row))
						return QString("-");
					return boxData[column - 1]->getDataValue(row);
				}
			} else {
				return QString("%");
			}
		} else if (role == Qt::TextColorRole) {
			if (coverage) {
				if (coverage[index.row()]) {
					if (box_coverage) {
						if (box_data > 0.75f)
							return DBG_GREEN;
						return DBG_RED;
					}
					return DBG_ORANGE;
				}
			} else {
				if (box_data > 0.75f)
					return DBG_GREEN;
				else if (box_data > 0.25f)
					return DBG_RED;
			}
		}
	} else {
		if (column >= boxData.size() || boxData.empty()
				|| row >= boxData[0]->getNumVertices())
			return QVariant();

		if (role == Qt::DisplayRole) {
			if (!boxData[column]->getCoverageValue(row))
				return QString("%");
			else if (!boxData[column]->getDataMapValue(row))
				return QString("-");
			return boxData[column]->getDataValue(row);
		}
	}

	return QVariant();
}

Qt::ItemFlags VertexTableModel::flags(const QModelIndex &index) const
{
	if (boxCondition) {
		if (!index.isValid() || index.column() >= boxData.size() + 1
				|| index.row() >= boxCondition->getNumVertices()) {
			return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
		}
		if (boxCondition->getCoveragePointer()[index.row()] == false) {
			return 0;
		}
	} else {
		if (!index.isValid() || index.column() >= boxData.size()
				|| boxData.empty()
				|| index.row() >= boxData[0]->getNumVertices()) {
			return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
		}

		if (!boxData.empty()
				&& boxData[0]->getCoveragePointer()[index.row()] == false) {
			return 0;
		}
	}
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant VertexTableModel::headerData(int section, Qt::Orientation orientation,
		int role) const
{
	if (role != Qt::DisplayRole) {
		return QVariant();
	} else if (orientation == Qt::Horizontal) {
		if (boxCondition) {
			if (section == 0)
				return QString("Condition");
			section--;
		}
		return names[section];
	}
	return section;
}

void VertexTableModel::detachData()
{
	VertexBox *vb = static_cast<VertexBox*>(sender());
	int idx = boxData.indexOf(vb);
	if (idx >= 0) {
		boxData.removeAt(idx);
		names.removeAt(idx);
		reset();
	}
	emit dataDeleted(idx);
	if (boxData.isEmpty())
		emit empty();
}

void VertexTableModel::updateData()
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

const QString& VertexTableModel::getDataColumnName(int column) const
{
	static const QString es("");
	if (column >= 0 && column < names.size())
		return names[column];
	return es;
}

VertexBox* VertexTableModel::getDataColumn(int column)
{
	if (column >= 0 && column < boxData.size())
		return boxData[column];
	return NULL;
}

