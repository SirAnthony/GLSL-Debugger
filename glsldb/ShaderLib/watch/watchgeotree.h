
#ifndef WATCH_GEOTREE_H
#define WATCH_GEOTREE_H

#include "data/vertexBox.h"
#include "watchview.h"
#include "glscatter.h"
#include "mappings.h"


class GeoShaderDataModel;
class GeoShaderDataSortFilterProxyModel;
class ShMappingWidget;

#define GT_WIDGETS_COUNT 6


namespace Ui {
	class ShWatchGeometry;
}


class WatchGeoTree: public WatchView
{
Q_OBJECT

public:
	WatchGeoTree(int inPrimitiveType, int outPrimitiveType,
			VertexBox *primitiveMap, VertexBox *vertexCount,
			QWidget *parent = 0);
	~WatchGeoTree();

	void updateView(bool force);
	virtual QAbstractItemModel * model();
	void attachData(VertexBox *currentData, VertexBox *vertexData,
			QString name);

public slots:
	virtual void connectDataBox(int);
	void updateData(int, int, float, float);
	void setBoundaries(int, double *, double *, bool);
	void closeView();

signals:
	void selectionChanged(int dataIdx);

private slots:
	void newSelection(const QModelIndex & index);

	void mappingDataChangedRed();
	void mappingDataChangedGreen();
	void mappingDataChangedBlue();
	void mappingDataChangedX();
	void mappingDataChangedY();
	void mappingDataChangedZ();

	void detachData(int idx);
	void on_tbDataSelection_clicked();
	void updatePointSize(int value);

private:
	Ui::ShWatchGeometry *ui;

	void addMappingOptions(int idx);
	void delMappingOptions(int idx);

	float getDataMin(int column);
	float getDataMax(int column);
	float getDataAbsMin(int column);
	float getDataAbsMax(int column);

	bool countsAllZero();
	void updateDataCurrent(float *data, int *count, int dataStride,
			VertexBox *srcData, RangeMapping *rangeMapping,
			float min, float max);
	void updateDataVertex(float *data, int *count, int dataStride,
			VertexBox *srcData, RangeMapping *rangeMapping,
			float min, float max);

	enum DataSelection {
		DATA_CURRENT,
		DATA_VERTEX
	} dataSelection;

	ShMappingWidget *widgets[GT_WIDGETS_COUNT];

	GeoShaderDataModel *_model;
	GeoShaderDataSortFilterProxyModel *modelFilter;

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
