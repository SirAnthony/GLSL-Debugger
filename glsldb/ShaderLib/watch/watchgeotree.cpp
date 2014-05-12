
#include <math.h>
#include <float.h>

#include "watchgeotree.h"
#include "ui_watchgeometry.h"
#include "shmappingwidget.h"
#include "geoShaderDataModel.qt.h"
#include "mappings.h"
#include "utils/dbgprint.h"


WatchGeoTree::WatchGeoTree(int inPrimitiveType, int outPrimitiveType,
		VertexBox *primitiveMap, VertexBox *vertexCount, QWidget *parent) :
		WatchView(parent)
{
	/* Setup GUI */
	ui->setupUi(this);

	fMapping->setVisible(false);

	_model = new GeoShaderDataModel(inPrimitiveType, outPrimitiveType,
			primitiveMap, vertexCount, NULL, NULL, this);
	modelFilter = new GeoShaderDataSortFilterProxyModel(this);
	modelFilter->setSourceModel(_model);
	modelFilter->setDynamicSortFilter(true);
	connect(tbHideInactive, SIGNAL(toggled(bool)), modelFilter,
			SLOT(setHideInactive(bool)));
	connect(tbHideEmpty, SIGNAL(toggled(bool)), modelFilter,
			SLOT(setHideEmpty(bool)));
	tvGeoData->setModel(modelFilter);
	tvGeoData->setAllColumnsShowFocus(true);
	tvGeoData->setUniformRowHeights(true);

	connect(tvGeoData, SIGNAL(doubleClicked(const QModelIndex &)), this,
			SLOT(newSelection(const QModelIndex &)));

	connect(_model, SIGNAL(dataDeleted(int)), this, SLOT(detachData(int)));
	connect(_model, SIGNAL(empty()), this, SLOT(closeView()));

	twGeoInfo->item(0, 0)->setText(QString(lookupEnum(inPrimitiveType)));
	twGeoInfo->item(1, 0)->setText(
			QString::number(_model->getNumInPrimitives()));
	twGeoInfo->item(0, 2)->setText(QString(lookupEnum(outPrimitiveType)));
	twGeoInfo->item(1, 2)->setText(
			QString::number(_model->getNumOutPrimitives()));

	if (GeoShaderDataModel::isBasicPrimitive(inPrimitiveType)) {
		twGeoInfo->hideColumn(1);
	} else {
		twGeoInfo->item(0, 1)->setText(
				lookupEnum(
						GeoShaderDataModel::getBasePrimitive(inPrimitiveType)));
		twGeoInfo->item(1, 1)->setText(
				QString::number(_model->getNumSubInPrimitives()));
	}

	twGeoInfo->resizeColumnsToContents();
	twGeoInfo->resizeRowsToContents();
	twGeoInfo->setSelectionBehavior(QAbstractItemView::SelectRows);

	// Add OpenGL view to window
	QGridLayout *gridLayout;
	gridLayout = new QGridLayout(fGLview);
	gridLayout->setSpacing(0);
	gridLayout->setMargin(0);
	m_qGLscatter = new GLScatter(this);
	gridLayout->addWidget(m_qGLscatter);

	slPointSize->setMinimum(1);
	slPointSize->setMaximum(1000);
	slPointSize->setValue(300);
	slPointSize->setTickInterval(50);

	scatterPositions = NULL;
	scatterColorsAndSizes = NULL;

	dataSelection = DATA_CURRENT;

	maxScatterDataElements = MAX(_model->getNumInPrimitives(),
			_model->getNumOutVertices());
	scatterDataElements = 0;
	scatterPositions = new float[3 * maxScatterDataElements];
	scatterColorsAndSizes = new float[3 * maxScatterDataElements];
	m_scatterDataX = scatterPositions;
	clearData(m_scatterDataX, maxScatterDataElements, 3, 0.0f);
	m_scatterDataCountX = 0;
	m_scatterDataY = scatterPositions + 1;
	clearData(m_scatterDataY, maxScatterDataElements, 3, 0.0f);
	m_scatterDataCountY = 0;
	m_scatterDataZ = scatterPositions + 2;
	clearData(m_scatterDataZ, maxScatterDataElements, 3, 0.0f);
	m_scatterDataCountZ = 0;
	m_scatterDataRed = scatterColorsAndSizes;
	clearData(m_scatterDataRed, maxScatterDataElements, 3, 0.0f);
	m_scatterDataCountRed = 0;
	m_scatterDataGreen = scatterColorsAndSizes + 1;
	clearData(m_scatterDataGreen, maxScatterDataElements, 3, 0.0f);
	m_scatterDataCountGreen = 0;
	m_scatterDataBlue = scatterColorsAndSizes + 2;
	clearData(m_scatterDataBlue, maxScatterDataElements, 3, 0.0f);
	m_scatterDataCountBlue = 0;
	m_qGLscatter->setData(scatterPositions, scatterColorsAndSizes, 0);
	updatePointSize(300);

	if (!scatterPositions) {
		maxScatterDataElements = vb->getNumVertices();
		scatterDataElements = 0;
		scatterPositions = new float[ELEMENTS_PER_VERTEX * maxScatterDataElements];
		scatterColorsAndSizes = new float[ELEMENTS_PER_VERTEX * maxScatterDataElements];
		for (int i = 0; i < WT_WIDGETS_COUNT; ++i) {
			scatterData[i] = scatterPositions + (i % ELEMENTS_PER_VERTEX);
			clearDataArray(scatterData[i], maxScatterDataElements,
						   scatterDataStride[i], 0.0f);
			scatterDataCount[i] = 0;
		}
	}

	updateGUI();
}

WatchGeoTree::~WatchGeoTree()
{
	m_qGLscatter->setData(scatterPositions, scatterColorsAndSizes, 0);
	delete[] scatterPositions;
	delete[] scatterColorsAndSizes;
}

bool WatchGeoTree::countsAllZero()
{
	for (int i = 0; i < GT_WIDGETS_COUNT; ++i)
		if (scatterDataCount[i] != 0)
			return false;
	return true;
}

void WatchGeoTree::updateDataCurrent(float *data, int *count,
		int dataStride, VertexBox *srcData,
		RangeMapping *rangeMapping, float min, float max)
{
	float minmax[2] = {
		min,
		fabs(max - min) > FLT_EPSILON ? max : 1.0f + min };
	float *dd = data;
	float *ds = srcData->getDataPointer();
	bool *dc = srcData->getCoveragePointer();
	bool *dm = srcData->getDataMapPointer();

	clearData(data, srcData->getNumVertices(), dataStride, 0.0f);

	*count = 0;
	for (int i = 0; i < srcData->getNumVertices(); i++) {
		if ((dc && !*dc) || (dm && !*dm)) {
		} else {
			*dd = getMappedValueF(*ds, rangeMapping, minmax);
			dd += dataStride;
			(*count)++;
		}
		ds++;
		if (dm)
			dm++;
		if (dc)
			dc++;
	}
	if (*count > 0) {
		scatterDataElements = *count;
	} else if (countsAllZero()) {
		scatterDataElements = 0;
	}

	m_qGLscatter->setData(scatterPositions, scatterColorsAndSizes,
			scatterDataElements);
}

void WatchGeoTree::updateDataVertex(float *data, int *count, int dataStride,
		VertexBox *srcData, RangeMapping *rangeMapping,
		float min, float max)
{
	float minmax[2] = {
		min,
		fabs(max - min) > FLT_EPSILON ? max : 1.0f + min };
	float *dd = data;
	float *ds = srcData->getDataPointer();
	bool *dm = srcData->getDataMapPointer();
	clearDataArray(data, srcData->getNumVertices(), dataStride, 0.0f);
	*count = 0;
	for (int i = 0; i < srcData->getNumVertices(); i++) {
		if (dm && !*dm) {
		} else if (ds[1] > 0.0f) {
			*dd = getMappedValueF(*ds, rangeMapping, minmax);
			dd += dataStride;
			(*count)++;
		}
		ds += 2;
		if (dm)
			dm++;
	}

	if (*count > 0) {
		scatterDataElements = *count;
	} else if (countsAllZero()) {
		scatterDataElements = 0;
	}

	m_qGLscatter->setData(scatterPositions, scatterColorsAndSizes,
			scatterDataElements);
}


void WatchGeoTree::attachData(VertexBox *currentData, VertexBox *vertexData,
		QString name)
{
	if (!currentData || !vertexData) {
		return;
	}

	if (_model->addData(currentData, vertexData, name)) {
		int idx = _model->columnCount() - 1;
		QString name = _model->getDataColumnName(idx);
		for (int i = 0; i < GT_WIDGETS_COUNT; ++i)
			widgets[i]->addOption(idx, name);

		updateGUI();
	}

	if (!vb)
		return;

	if (_model->addVertexBox(vb, name)) {
		int idx = _model->columnCount() - 1;
		QString name = _model->getDataColumnName(idx);
		for (int i = 0; i < WT_WIDGETS_COUNT; ++i)
			widgets[i]->addOption(idx, name);

		updateGUI();
	}
}

void WatchGeoTree::connectDataBox(int index)
{
	ShMappingWidget* widget = dynamic_cast<ShMappingWidget*>(sender());
	VertexBox *vb = NULL;
	switch (dataSelection) {
	case DATA_CURRENT:
		vb = _model->getDataColumnCurrentData(index);
		break;
	case DATA_VERTEX:
		vb = _model->getDataColumnVertexData(index);
		break;
	default:
		Q_ASSERT_X(0, "WatchGeoTree::connectDataBox", "Wrong dataSelection");
		return;
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

	switch (dataSelection) {
	case DATA_CURRENT:
		updateDataCurrent(scatterData[idx], &scatterDataCount[idx],
						  scatterDataStride[idx], _model->getDataColumnCurrentData(index),
						  static_cast<RangeMap>(range), min, max);
		break;
	case DATA_VERTEX:
		updateDataVertex(scatterData[idx], &scatterDataCount[idx],
						 scatterDataStride[idx], _model->getDataColumnVertexData(index),
						 static_cast<RangeMap>(range), min, max);
		break;
	default:
		Q_ASSERT_X(0, "WatchGeoTree::updateData", "Wrong dataSelection");
		return;
	}
}

void WatchGeoTree::clearData()
{
	ShMappingWidget *sndr = static_cast<ShMappingWidget*>(sender());
	int idx = WT_WIDGETS_COUNT;
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
	switch (dataSelection) {
		case DATA_CURRENT:
			*box = _model->getDataColumnCurrentData(index);
			break;
		case DATA_VERTEX:
			*box = _model->getDataColumnVertexData(index);
			break;
	}
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

void WatchGeoTree::closeView()
{
	hide();
	deleteLater();
}

void WatchGeoTree::newSelection(const QModelIndex & index)
{
	if (index.isValid()) {
		int dataIdx = index.data(GeoShaderDataModel::IndexRole).toInt();
		if (dataIdx >= 0
				&& !index.data(GeoShaderDataModel::VertexRole).toBool()) {
			emit selectionChanged(dataIdx);
		}
	}
}

void WatchGeoTree::on_tbDataSelection_clicked()
{
	/* TODO: switch between display of current and emitted vertex values */
	/* change icon, etc. */
	if (dataSelection == DATA_VERTEX) {
		dataSelection = DATA_CURRENT;
	} else if (dataSelection == DATA_CURRENT) {
		dataSelection = DATA_VERTEX;
	}
	clearData(m_scatterDataX, maxScatterDataElements, 3, 0.0f);
	clearData(m_scatterDataY, maxScatterDataElements, 3, 0.0f);
	clearData(m_scatterDataZ, maxScatterDataElements, 3, 0.0f);
	clearData(m_scatterDataRed, maxScatterDataElements, 3, 0.0f);
	clearData(m_scatterDataGreen, maxScatterDataElements, 3, 0.0f);
	clearData(m_scatterDataBlue, maxScatterDataElements, 3, 0.0f);
	m_scatterDataCountX = 0;
	m_scatterDataCountY = 0;
	m_scatterDataCountZ = 0;
	m_scatterDataCountRed = 0;
	m_scatterDataCountGreen = 0;
	m_scatterDataCountBlue = 0;
	on_cbX_activated(cbX->currentIndex());
	on_cbY_activated(cbY->currentIndex());
	on_cbZ_activated(cbZ->currentIndex());
	on_cbRed_activated(cbRed->currentIndex());
	on_cbGreen_activated(cbGreen->currentIndex());
	on_cbBlue_activated(cbBlue->currentIndex());
	m_qGLscatter->updateGL();
	updateView(true);
}

float WatchGeoTree::getDataMin(int column)
{
	switch (dataSelection) {
	case DATA_CURRENT:
		return _model->getDataColumnCurrentData(column)->getMin();
	case DATA_VERTEX: {
		float min = FLT_MAX;
		VertexBox *vb = _model->getDataColumnVertexData(column);
		float *pData = vb->getDataPointer();
		bool *pDataMap = vb->getDataMapPointer();

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

		float *pData = vb->getDataPointer();
		bool *pDataMap = vb->getDataMapPointer();

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
		float *pData = vb->getDataPointer();
		bool *pDataMap = vb->getDataMapPointer();

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
		float *pData = vb->getDataPointer();
		bool *pDataMap = vb->getDataMapPointer();

		for (int i = 0; i < vb->getNumVertices(); i++) {
			if (*pDataMap && pData[1] > 0.0f && max < fabs(pData[0])) {
				max = fabs(pData[0]);
			}
			pData += 2;
			pDataMap += 1;
		}
		return max;
	}
	}
	return 0.0;
}

void WatchGeoTree::updatePointSize(int value)
{
	float newValue = 0.03 * (float) value / (float) ui->slPointSize->maximum();
	ui->lbPointSize->setText(QString::number(newValue));
	ui->glScatter->setPointSize(newValue);
	ui->glScatter->updateGL();
}

