
#ifndef WATCH_GEOTREE_H
#define WATCH_GEOTREE_H

#include "data/vertexBox.h"
#include "watchview.h"
#include "glscatter.h"
#include "mappings.h"
#include <QModelIndex>

class GeometryDataModel;
class GeometryDataSortFilterProxyModel;
class ShMappingWidget;
class GeometryInfo;

#define GT_WIDGETS_COUNT 6


namespace Ui {
	class ShWatchGeoTree;
}


class WatchGeoTree: public WatchView
{
Q_OBJECT

public:
	WatchGeoTree(GeometryInfo *info, QWidget *parent = 0);
	~WatchGeoTree();

	virtual void updateView(bool force);
	virtual QAbstractItemModel * model();
	virtual void attachData(DataBox *, QString &) {}
	virtual void attachData(DataBox *, DataBox *, QString &);

public slots:
	virtual void updateData(int, int, float, float);
	virtual void clearData();
	virtual void setBoundaries(int, double *, double *, bool);
	virtual void setDataBox(int, DataBox **);

signals:
	void selectionChanged(int dataIdx);

private slots:
	void newSelection(const QModelIndex & index);
	void detachData(int idx);
	void changeDataSelection();
	void updatePointSize(int value);

private:
	Ui::ShWatchGeoTree *ui;

	void updateDataInfo(int primitiveIn, int primitiveOut);
	float getDataMin(int column);
	float getDataMax(int column);
	float getDataAbsMin(int column);
	float getDataAbsMax(int column);
	VertexBox *getDataColumn(int index);

	void initScatter(int elements);
	bool countsAllZero();
	void updateDataCurrent(float *data, int *count, int dataStride,
			VertexBox *srcData, RangeMap range, float min, float max);

	enum DataSelection {
		DATA_CURRENT,
		DATA_VERTEX
	} dataSelection;

	ShMappingWidget *widgets[GT_WIDGETS_COUNT];

	GeometryDataModel *_model;
	GeometryDataSortFilterProxyModel *modelFilter;

	float *scatterPositions;
	float *scatterColorsAndSizes;
	int maxScatterDataElements;
	int scatterDataElements;

	float *scatterData[GT_WIDGETS_COUNT];
	int scatterDataCount[GT_WIDGETS_COUNT];
	const int scatterDataStride[GT_WIDGETS_COUNT] = {
		3, 3, 3, 3, 3, 3
	};
};

#endif /* WATCH_GEOTREE_H */
