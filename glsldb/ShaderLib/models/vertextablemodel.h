
#ifndef VERTEX_TABLE_MODEL_H
#define VERTEX_TABLE_MODEL_H

#include <QtCore/QAbstractTableModel>
#include <QtGui/QSortFilterProxyModel>
#include "data/vertexBox.h"
#include "shtabledatamodel.h"

class VertexTableSortFilterProxyModel: public QSortFilterProxyModel {
Q_OBJECT

public:
	VertexTableSortFilterProxyModel(QObject *parent = 0);

public slots:
	void setHideInactive(bool);

protected:
	virtual bool filterAcceptsRow(int, const QModelIndex &) const;

private:
	bool hideInactive;
};



class VertexTableModel: public QAbstractTableModel, public ShTableDataModel {
Q_OBJECT

public:
	VertexTableModel(VertexBox *, const bool *cov = NULL, QObject *parent = 0);
	~VertexTableModel();

	virtual bool addItem(ShVarItem *);
	bool addData(VertexBox *vertex, QString& name);
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent = QModelIndex()) const;

	QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	Qt::ItemFlags flags(const QModelIndex &index) const;
	QVariant headerData(int section, Qt::Orientation orientation,
						int role = Qt::DisplayRole) const;
	const QString &getDataColumnName(int column) const;
	VertexBox* getDataColumn(int column);

public slots:
	void updateData();

signals:
	void empty();
	void dataDeleted(int idx);

private slots:
	void detachData();

private:
	VertexTableSortFilterProxyModel *proxy;

	QList<VertexBox *> boxData;
	QList<QString> names;
	VertexBox *boxCondition;
	const bool *coverage;
};

#endif /* VERTEX_TABLE_MODEL_H */

