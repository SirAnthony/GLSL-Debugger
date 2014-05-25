
#ifndef WATCHTABLE_H
#define WATCHTABLE_H

#include "data/vertexBox.h"
#include "watchview.h"
#include "mappings.h"
#include <QModelIndex>

#define WT_WIDGETS_COUNT 6

class VertexTableModel;
class VertexTableSortFilterProxyModel;
class GLScatter;
class ShMappingWidget;

namespace Ui {
	class ShWatchTable;
}

class WatchTable: public WatchView
{
Q_OBJECT

public:
	WatchTable(QWidget *parent = 0);
	~WatchTable();
	virtual void updateView(bool force);
	virtual QAbstractItemModel * model();
	virtual void attachData(DataBox *, QString &);

public slots:
	virtual void updateData(int, int, float, float);
	virtual void clearData();
	virtual void setBoundaries(int, double *, double *, bool);
	virtual void setDataBox(int, DataBox **);
	void detachData(int idx);
	void newSelection(const QModelIndex & index);
	void updatePointSize(int value);

private:
	Ui::ShWatchTable *ui;

	void initScatter(int elements);
	bool countsAllZero();
	void updateDataCurrent(float *data, int *count, int dataStride,
			VertexBox *srcData, RangeMap range, float min, float max);

	ShMappingWidget *widgets[WT_WIDGETS_COUNT];

	VertexTableModel *_model;
	VertexTableSortFilterProxyModel *_modelFilter;

	float *scatterPositions;
	float *scatterColorsAndSizes;
	int maxScatterDataElements;
	int scatterDataElements;

	float *scatterData[WT_WIDGETS_COUNT];
	int scatterDataCount[WT_WIDGETS_COUNT];
	const int scatterDataStride[WT_WIDGETS_COUNT] = {
		3, 3, 3, 3, 3, 3
	};
};

#endif /* WATCHTABLE_H */

