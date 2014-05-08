
#ifndef WATCHTABLE_H
#define WATCHTABLE_H

#include "data/vertexBox.h"
#include "watchview.h"
#include "mappings.h"

#define WT_WIDGETS_COUNT 6

class VertexTableModel;
class VertexTableSortFilterProxyModel;
class GLScatter;
class ShMappingWidget;

namespace Ui {
	class ShWatchTable;
}

class WatchTable: public WatchView {
Q_OBJECT

public:
	WatchTable(QWidget *parent = 0);
	~WatchTable();
	void updateView(bool force);
	void attachVpData(VertexBox *f, QString name);

public slots:
	void closeView();

signals:
	void selectionChanged(int vertId);

private slots:	
	void updateData(int, int, float, float);
	void detachData(int idx);
	void clearData();
	void newSelection(const QModelIndex & index);
	void updatePointSize(int value);


private:
	Ui::ShWatchTable *ui;

	void updateGUI();
	bool countsAllZero();
	void updateDataCurrent(float *data, int *count, int dataStride,
			VertexBox *srcData, RangeMap range, float min, float max);

	ShMappingWidget *widgets[WT_WIDGETS_COUNT];

	VertexTableModel *model;
	VertexTableSortFilterProxyModel *modelFilter;

	float *scatterPositions;
	float *scatterColorsAndSizes;
	int maxScatterDataElements;
	int scatterDataElements;

	float *scatterData[WT_WIDGETS_COUNT];
	int scatterDataCount[WT_WIDGETS_COUNT];
	static const int scatterDataStride[WT_WIDGETS_COUNT] = {
		3, 3, 3, 3, 3, 3
	};
};

#endif /* WATCHTABLE_H */

