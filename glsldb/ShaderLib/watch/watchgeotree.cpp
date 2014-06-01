
#include <math.h>
#include <float.h>

#include "shwindowmanager.h"
#include "watchgeotree.h"
#include "ui_watchgeometry.h"
#include "shmappingwidget.h"
#include "mappings.h"
#include "shdatamanager.h"
#include "models/geometrydatamodel.h"
#include "utils/dbgprint.h"

WatchGeoTree::WatchGeoTree(const GeometryInfo &info, QWidget *parent) :
		WatchView(parent)
{
	/* Setup GUI */
	ui->setupUi(this);
	ui->fMapping->setVisible(false);
	type = ShWindowManager::wtGeometry;

	_model = new GeometryDataModel(info, NULL, NULL, this);
	modelFilter = new GeometryDataSortFilterProxyModel(this);
	modelFilter->setSourceModel(_model);
	modelFilter->setDynamicSortFilter(true);
	connect(ui->tbHideInactive, SIGNAL(toggled(bool)), modelFilter,
			SLOT(setHideInactive(bool)));
	connect(ui->tbHideEmpty, SIGNAL(toggled(bool)), modelFilter,
			SLOT(setHideEmpty(bool)));

	ui->tvGeoData->setModel(modelFilter);
	ui->tvGeoData->setAllColumnsShowFocus(true);
	ui->tvGeoData->setUniformRowHeights(true);

	connect(ui->tvGeoData, SIGNAL(doubleClicked(const QModelIndex &)), this,
			SLOT(newSelection(const QModelIndex &)));

	connect(_model, SIGNAL(dataDeleted(int)), this, SLOT(detachData(int)));
	connect(_model, SIGNAL(empty()), this, SLOT(closeView()));

	for (int i = 0; i < GT_WIDGETS_COUNT; ++i)
		connectWidget(widgets[i]);

	updateDataInfo(info.primitiveMode, info.outputType);

	dataSelection = DATA_CURRENT;
	initScatter(std::max(_model->getNumInPrimitives(), _model->getNumOutVertices()));
	updatePointSize(300);
	updateGUI();
}

WatchGeoTree::~WatchGeoTree()
{
	delete[] scatterPositions;
	delete[] scatterColorsAndSizes;
}

void WatchGeoTree::attachData(DataBox *cbox, DataBox *dbox, QString &name)
{
	VertexBox* currentData = dynamic_cast<VertexBox *>(cbox);
	VertexBox* vertexData = dynamic_cast<VertexBox *>(dbox);
	if (!currentData || !vertexData)
		return;

	if (_model->addData(currentData, vertexData, name)) {
		int idx = _model->columnCount() - 1;
		QString name = _model->getDataColumnName(idx);
		for (int i = 0; i < GT_WIDGETS_COUNT; ++i)
			widgets[i]->addOption(idx, name);

		updateGUI();
	}
}

void WatchGeoTree::updateData(int index, int range, float min, float max)
{
	ShMappingWidget *sndr = static_cast<ShMappingWidget*>(sender());
	int idx = GT_WIDGETS_COUNT;
	while (--idx > 0)
		if (sndr == widgets[idx])
			break;

	if (idx < 0)
		return;

	VertexBox* data = getDataColumn(index);
	updateDataCurrent(scatterData[idx], &scatterDataCount[idx], scatterDataStride[idx],
					  data, static_cast<RangeMap>(range), min, max);
}

void WatchGeoTree::clearData()
{
	ShMappingWidget *sndr = static_cast<ShMappingWidget*>(sender());
	int idx = GT_WIDGETS_COUNT;
	while (--idx > 0)
		if (sndr == widgets[idx])
			break;

	if (idx < 0)
		return;

	clearDataArray(scatterData[idx], maxScatterDataElements,
				   scatterDataStride[idx], 0.0f);
}

void WatchGeoTree::setBoundaries(int index, double *min, double *max, bool absolute)
{
	if (absolute) {
		*min = getDataAbsMin(index);
		*max = getDataAbsMax(index);
	} else {
		*min = getDataMin(index);
		*max = getDataMax(index);
	}
}

void WatchGeoTree::setDataBox(int index, DataBox **box)
{
	*box = getDataColumn(index);
}


void WatchGeoTree::detachData(int idx)
{
	dbgPrint(DBGLVL_DEBUG, "WatchGeoTree::detachData: idx=%i\n", idx);
	for (int i = 0; i < GT_WIDGETS_COUNT; ++i)
		widgets[i]->delOption(idx);
}

void WatchGeoTree::updateView(bool force)
{
	UNUSED_ARG(force)
			/* TODO */
}

QAbstractItemModel *WatchGeoTree::model()
{
	return _model;
}

void WatchGeoTree::newSelection(const QModelIndex & index)
{
	if (index.isValid()) {
		int dataIdx = index.data(GeometryDataModel::IndexRole).toInt();
		if (dataIdx >= 0
				&& !index.data(GeometryDataModel::VertexRole).toBool()) {
			emit selectionChanged(dataIdx, -1);
		}
	}
}

void WatchGeoTree::changeDataSelection()
{
	/* TODO: switch between display of current and emitted vertex values */
	/* change icon, etc. */
	if (dataSelection == DATA_VERTEX) {
		dataSelection = DATA_CURRENT;
	} else if (dataSelection == DATA_CURRENT) {
		dataSelection = DATA_VERTEX;
	}

	for (int i = 0; i < GT_WIDGETS_COUNT; ++i) {
		clearDataArray(scatterData[i], maxScatterDataElements, ELEMENTS_PER_VERTEX, 0.0f);
		scatterDataCount[i] = 0;
		widgets[i]->cbValActivated(widgets[i]->currentValIndex());
	}
}

void WatchGeoTree::updatePointSize(int value)
{
	float newValue = 0.03 * (float) value / (float) ui->slPointSize->maximum();
	ui->lbPointSize->setText(QString::number(newValue));
	ui->glScatter->setPointSize(newValue);
	ui->glScatter->updateGL();
}

void WatchGeoTree::updateDataInfo(int primitiveIn, int primitiveOut)
{
	ui->twGeoInfo->item(0, 0)->setText(GeometryDataModel::primitiveString(primitiveIn));
	ui->twGeoInfo->item(1, 0)->setText(QString::number(_model->getNumInPrimitives()));
	ui->twGeoInfo->item(0, 2)->setText(GeometryDataModel::primitiveString(primitiveOut));
	ui->twGeoInfo->item(1, 2)->setText(QString::number(_model->getNumOutPrimitives()));

	if (GeometryDataModel::isBasicPrimitive(primitiveIn)) {
		ui->twGeoInfo->hideColumn(1);
	} else {
		ui->twGeoInfo->item(0, 1)->setText(GeometryDataModel::primitiveString(
								GeometryDataModel::getBasePrimitive(primitiveIn)));
		ui->twGeoInfo->item(1, 1)->setText(QString::number(_model->getNumSubInPrimitives()));
	}
	ui->twGeoInfo->resizeColumnsToContents();
	ui->twGeoInfo->resizeRowsToContents();
}

float WatchGeoTree::getDataMin(int column)
{
	switch (dataSelection) {
	case DATA_CURRENT:
		return _model->getDataColumnCurrentData(column)->getMin();
	case DATA_VERTEX: {
		float min = FLT_MAX;
		VertexBox *vb = _model->getDataColumnVertexData(column);
		const float *pData = static_cast<const float*>(vb->getDataPointer());
		const bool *pDataMap = vb->getDataMapPointer();

		for (int i = 0; i < vb->getNumVertices(); i++) {
			if (*pDataMap && pData[1] > 0.0f && min > pData[0]) {
				min = pData[0];
			}
			pData += 2;
			pDataMap += 1;
		}
		return min;
	}
	}
	return 0.0;
}

float WatchGeoTree::getDataMax(int column)
{
	switch (dataSelection) {
	case DATA_CURRENT:
		return _model->getDataColumnCurrentData(column)->getMax();
	case DATA_VERTEX: {
		float max = -FLT_MAX;
		VertexBox *vb = _model->getDataColumnVertexData(column);

		const float *pData = static_cast<const float*>(vb->getDataPointer());
		const bool *pDataMap = vb->getDataMapPointer();

		for (int i = 0; i < vb->getNumVertices(); i++) {
			if (*pDataMap && pData[1] > 0.0f && max < pData[0]) {
				max = pData[0];
			}
			pData += 2;
			pDataMap += 1;
		}
		return max;
	}
	}
	return 0.0;
}

float WatchGeoTree::getDataAbsMin(int column)
{
	switch (dataSelection) {
	case DATA_CURRENT:
		return _model->getDataColumnCurrentData(column)->getAbsMin();
		break;
	case DATA_VERTEX: {
		float min = FLT_MAX;
		VertexBox *vb = _model->getDataColumnVertexData(column);
		const float *pData = static_cast<const float*>(vb->getDataPointer());
		const bool *pDataMap = vb->getDataMapPointer();

		for (int i = 0; i < vb->getNumVertices(); i++) {
			if (*pDataMap && pData[1] > 0.0f && min > fabs(pData[0])) {
				min = fabs(pData[0]);
			}
			pData += 2;
			pDataMap += 1;
		}
		return min;
	}
	}
	return 0.0;
}

float WatchGeoTree::getDataAbsMax(int column)
{
	switch (dataSelection) {
	case DATA_CURRENT:
		return _model->getDataColumnCurrentData(column)->getAbsMax();
		break;
	case DATA_VERTEX: {
		float max = 0.0;
		VertexBox *vb = _model->getDataColumnVertexData(column);
		const float *pData = static_cast<const float*>(vb->getDataPointer());
		const bool *pDataMap = vb->getDataMapPointer();
		for (int i = 0; i < vb->getNumVertices(); i++) {
			if (*pDataMap && pData[1] > 0.0f && max < fabs(pData[0]))
				max = fabs(pData[0]);
			pData += 2;
			pDataMap += 1;
		}
		return max;
	}
	}
	return 0.0;
}

VertexBox * WatchGeoTree::getDataColumn(int index)
{
	switch (dataSelection) {
	case DATA_CURRENT:
		return _model->getDataColumnCurrentData(index);
	case DATA_VERTEX:
		return _model->getDataColumnVertexData(index);
	default:
		Q_ASSERT_X(0, "WatchGeoTree::updateData", "Wrong dataSelection");
		return NULL;
	}
}

void WatchGeoTree::initScatter(int elements)
{
	if (scatterPositions)
		return;

	maxScatterDataElements = elements;
	scatterDataElements = 0;
	scatterPositions = new float[ELEMENTS_PER_VERTEX * maxScatterDataElements];
	scatterColorsAndSizes = new float[ELEMENTS_PER_VERTEX * maxScatterDataElements];
	for (int i = 0; i < GT_WIDGETS_COUNT; ++i) {
		float *source = i < ELEMENTS_PER_VERTEX ? scatterPositions : scatterColorsAndSizes;
		scatterData[i] = source + (i % ELEMENTS_PER_VERTEX);
		clearDataArray(scatterData[i], maxScatterDataElements,
					   scatterDataStride[i], 0.0f);
		scatterDataCount[i] = 0;
	}
}

bool WatchGeoTree::countsAllZero()
{
	for (int i = 0; i < GT_WIDGETS_COUNT; ++i)
		if (scatterDataCount[i] != 0)
			return false;
	return true;
}

void WatchGeoTree::updateDataCurrent(float *data, int *count, int dataStride,
		VertexBox *srcData, RangeMap range, float min, float max)
{
	float *pData = data;
	const float *pSourceData = static_cast<const float*>(srcData->getDataPointer());
	const bool *pCov = srcData->getCoveragePointer();
	const bool *pMap = srcData->getDataMapPointer();
	int verticles = srcData->getNumVertices();
	clearDataArray(data, verticles, dataStride, 0.0f);
	if (fabs(max - min) <= FLT_EPSILON)
		max = 1.0f + min;

	bool vertex = dataSelection == DATA_VERTEX;
	int source_increment = vertex ? 2 : 1;

	int total = 0;
	for (int i = 0; i < verticles; i++, pSourceData += source_increment) {
		bool vCov = vertex && pCov && !pCov[i];
		bool cCov = (pMap && !pMap[i]) || (vertex && pSourceData[1] <= 0.0);
		if (vCov || cCov)
			continue;
		*pData = getMappedValueF(*pSourceData, range, min, max);
		pData += dataStride;
		total++;
	}

	*count = total;
	if (total > 0)
		scatterDataElements = total;
	else if (countsAllZero())
		scatterDataElements = 0;

	ui->glScatter->setData(scatterPositions, scatterColorsAndSizes,
						   scatterDataElements, ELEMENTS_PER_VERTEX);
}



