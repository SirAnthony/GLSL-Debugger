
#ifndef GEOMETRYDATAMODEL_H
#define GEOMETRYDATAMODEL_H

#include <QSortFilterProxyModel>
#include <QModelIndex>
#include <QVariant>
#include <QAbstractItemModel>
#include "shdatamanager.h"
#include "shtabledatamodel.h"


class VertexBox;
class GeometryTreeItem;


class GeometryDataSortFilterProxyModel: public QSortFilterProxyModel
{
Q_OBJECT
public:
	GeometryDataSortFilterProxyModel(QObject *parent = 0);

public slots:
	void setHideInactive(bool b);
	void setHideEmpty(bool b);

protected:
	virtual bool filterAcceptsRow(int row, const QModelIndex &parent) const;

private:
	bool hideInactive;
	bool hideEmpty;
};



class GeometryDataModel: public QAbstractItemModel, public ShTableDataModel {
Q_OBJECT

public:
	GeometryDataModel(const GeometryInfo &geometry, VertexBox *condition,
					  const bool *initial = NULL, QObject *parent = 0);
	~GeometryDataModel();

	virtual bool addItem(ShVarItem *);
	bool addData(VertexBox *current, VertexBox *vertex, QString &name);

	QVariant data(const QModelIndex &index, int role) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role =
			Qt::DisplayRole) const;
	QModelIndex index(int row, int column, const QModelIndex &parent =
			QModelIndex()) const;
	QModelIndex parent(const QModelIndex &index) const;
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;
	bool noOutputPrims(const QModelIndex &index) const;

	static QString primitiveString(int type);
	static bool isBasicPrimitive(int primType);
	static int getBasePrimitive(int type);

	int getNumInPrimitives() const;
	int getNumSubInPrimitives() const;
	int getNumOutPrimitives() const;
	int getNumOutVertices() const;

	int getDataColumnCount() const;
	const QString &getDataColumnName(int column) const;
	VertexBox* getDataColumnVertexData(int column);
	VertexBox* getDataColumnCurrentData(int column);

	enum DataRoles {
		VertexRole = Qt::UserRole,
		IndexRole = Qt::UserRole + 1
	};

signals:
	void empty();
	void dataDeleted(int i);

private slots:
	void updateData();
	void detachData();

protected:
	int generateVerticles(const float *data, int offset, int count,
						  int total, GeometryTreeItem *primitive);

private:
	int inPrimitive;
	int outPrimitive;
	int numInPrimitives;
	int numSubInPrimitives;
	int numOutPrimitives;
	int numOutVertices;
	VertexBox *condition;
	const bool *coverage;
	QList<VertexBox*> currentData;
	QList<VertexBox*> vertexData;
	QList<QString> dataNames;

	GeometryTreeItem *rootItem;
};

#endif /* GEOMETRYDATAMODEL_H */

