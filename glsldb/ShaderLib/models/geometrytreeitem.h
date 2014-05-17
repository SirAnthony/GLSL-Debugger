
#ifndef GEOMETRYTREEITEM_H
#define GEOMETRYTREEITEM_H

#include <QtCore/QList>
#include <QtCore/QVariant>
#include <QStandardItem>

class VertexBox;


class GeometryTreeItem : public QStandardItem {
public:
	GeometryTreeItem(QString name, int dataIdx, QList<VertexBox*> *data);
	virtual ~GeometryTreeItem();
	virtual int columnCount() const;
	virtual QVariant data(int column) const;
	virtual Qt::ItemFlags flags(int column) const;
	virtual QVariant displayColor(int column) const;
	virtual GeometryTreeItem *parent() const;

	int getDataIndex() const;
	virtual bool isVertexItem() const
	{
		return false;
	}

protected:
	QString itemName;
	int dataIdx;
	QList<VertexBox *> *dataList;
};

class GeometryTreeInPrimitive: public GeometryTreeItem {
public:
	GeometryTreeInPrimitive(QString name, int dataIdx, QList<VertexBox*> *data,
			VertexBox *condition, bool *initialCondition);

	virtual int columnCount() const;
	virtual QVariant data(int column) const;
	virtual QVariant displayColor(int column) const;
	virtual Qt::ItemFlags flags(int column) const;

protected:
	VertexBox *condition;
	bool *initialCondition;
};

class GeometryTreeOutPrimitive: public GeometryTreeItem {
public:
	GeometryTreeOutPrimitive(QString name);

	virtual QVariant data(int column) const;
	virtual Qt::ItemFlags flags(int column) const;
};

class GeometryTreeVertex: public GeometryTreeItem {
public:
	GeometryTreeVertex(QString name, int dataIdx, QList<VertexBox*> *data);

	virtual QVariant data(int column) const;
	virtual Qt::ItemFlags flags(int column) const;
	virtual bool isVertexItem() const
	{
		return true;
	}
};

#endif /* GEOMETRYTREEITEM_H */
