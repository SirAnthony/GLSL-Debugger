
#include <math.h>
#include <float.h>

#include "watchgeotree.h"
#include "ui_watchgeometry.h"
#include "shmappingwidget.h"
#include "geoShaderDataModel.qt.h"
#include "mappings.h"
#include "dbgprint.h"

extern "C" {
#include "DebugLib/glenumerants.h"
}

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

static void clearData(float *data, int count, int dataStride, float clearValue)
{
	for (int i = 0; i < count; i++) {
		*data = clearValue;
		data += dataStride;
	}
}

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

	setupMappingUI();

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
	if (m_scatterDataCountX != 0) {
		return false;
	}
	if (m_scatterDataCountY != 0) {
		return false;
	}
	if (m_scatterDataCountZ != 0) {
		return false;
	}
	if (m_scatterDataCountRed != 0) {
		return false;
	}
	if (m_scatterDataCountGreen != 0) {
		return false;
	}
	if (m_scatterDataCountBlue != 0) {
		return false;
	}
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

	clearData(data, srcData->getNumVertices(), dataStride, 0.0f);

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

#define UPDATE_DATA(VAL) \
    if (cb##VAL->currentIndex() == 0) { \
        clearData(m_scatterData##VAL, m_maxScatterDataElements, \
                m_scatterDataStride##VAL, 0.0f); \
    } else { \
        Mapping m = getMappingFromInt(cb##VAL->itemData(cb##VAL->currentIndex()).toInt()); \
        RangeMapping rm = getRangeMappingFromInt(cbMap##VAL->itemData(cbMap##VAL->currentIndex()).toInt()); \
        switch (m_dataSelection) { \
            case DATA_CURRENT: \
                updateDataCurrent(m_scatterData##VAL, &m_scatterDataCount##VAL, \
                        m_scatterDataStride##VAL, \
                        m_dataModel->getDataColumnCurrentData(m.index), &m, &rm, \
                        dsMin##VAL->value(), dsMax##VAL->value()); \
                break; \
            case DATA_VERTEX: \
                updateDataVertex(m_scatterData##VAL, &m_scatterDataCount##VAL, \
                        m_scatterDataStride##VAL, \
                        m_dataModel->getDataColumnVertexData(m.index), &m, &rm, \
                        dsMin##VAL->value(), dsMax##VAL->value()); \
                break; \
        } \
    }

void WatchGeoTree::attachData(VertexBox *currentData, VertexBox *vertexData,
		QString name)
{
	if (!currentData || !vertexData) {
		return;
	}

	if (_model->addData(currentData, vertexData, name)) {
		addMappingOptions(_model->getDataColumnCount() - 1);
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

void WatchGeoTree::addMappingOptions(int idx)
{
	Mapping m;
	m.type = MAP_TYPE_VAR;
	m.index = idx;

	QString name = _model->getDataColumnName(idx);
	cbRed->addItem(name, QVariant(getIntFromMapping(m)));
	cbGreen->addItem(name, QVariant(getIntFromMapping(m)));
	cbBlue->addItem(name, QVariant(getIntFromMapping(m)));
	cbX->addItem(name, QVariant(getIntFromMapping(m)));
	cbY->addItem(name, QVariant(getIntFromMapping(m)));
	cbZ->addItem(name, QVariant(getIntFromMapping(m)));
}

void WatchGeoTree::delMappingOptions(int idx)
{
	/* Check if it's in use right now */
	QVariant data;
	Mapping m;

	data = cbRed->itemData(cbRed->currentIndex());
	m = getMappingFromInt(data.toInt());
	if (m.index == idx) {
		cbRed->setCurrentIndex(0);
		on_cbRed_activated(0);
	}
	data = cbGreen->itemData(cbGreen->currentIndex());
	m = getMappingFromInt(data.toInt());
	if (m.index == idx) {
		cbGreen->setCurrentIndex(0);
		on_cbGreen_activated(0);
	}
	data = cbBlue->itemData(cbBlue->currentIndex());
	m = getMappingFromInt(data.toInt());
	if (m.index == idx) {
		cbBlue->setCurrentIndex(0);
		on_cbBlue_activated(0);
	}
	data = cbX->itemData(cbX->currentIndex());
	m = getMappingFromInt(data.toInt());
	if (m.index == idx) {
		cbX->setCurrentIndex(0);
		on_cbX_activated(0);
	}
	data = cbY->itemData(cbY->currentIndex());
	m = getMappingFromInt(data.toInt());
	if (m.index == idx) {
		cbY->setCurrentIndex(0);
		on_cbY_activated(0);
	}
	data = cbZ->itemData(cbZ->currentIndex());
	m = getMappingFromInt(data.toInt());
	if (m.index == idx) {
		cbZ->setCurrentIndex(0);
		on_cbZ_activated(0);
	}

	/* Delete options in comboboxes */
	int map;
	m.index = idx;

	m.type = MAP_TYPE_VAR;
	map = getIntFromMapping(m);
	idx = cbRed->findData(QVariant(map));
	cbRed->removeItem(idx);
	for (int i = idx; i < cbRed->count(); i++) {
		m = getMappingFromInt(cbRed->itemData(i).toInt());
		m.index--;
		cbRed->setItemData(i, getIntFromMapping(m));
	}
	idx = cbGreen->findData(QVariant(map));
	cbGreen->removeItem(idx);
	for (int i = idx; i < cbGreen->count(); i++) {
		m = getMappingFromInt(cbGreen->itemData(i).toInt());
		m.index--;
		cbGreen->setItemData(i, getIntFromMapping(m));
	}
	idx = cbBlue->findData(QVariant(map));
	cbBlue->removeItem(idx);
	for (int i = idx; i < cbBlue->count(); i++) {
		m = getMappingFromInt(cbBlue->itemData(i).toInt());
		m.index--;
		cbBlue->setItemData(i, getIntFromMapping(m));
	}
	idx = cbX->findData(QVariant(map));
	cbX->removeItem(idx);
	for (int i = idx; i < cbX->count(); i++) {
		m = getMappingFromInt(cbX->itemData(i).toInt());
		m.index--;
		cbX->setItemData(i, getIntFromMapping(m));
	}
	idx = cbY->findData(QVariant(map));
	cbY->removeItem(idx);
	for (int i = idx; i < cbY->count(); i++) {
		m = getMappingFromInt(cbY->itemData(i).toInt());
		m.index--;
		cbY->setItemData(i, getIntFromMapping(m));
	}
	idx = cbZ->findData(QVariant(map));
	cbZ->removeItem(idx);
	for (int i = idx; i < cbZ->count(); i++) {
		m = getMappingFromInt(cbZ->itemData(i).toInt());
		m.index--;
		cbZ->setItemData(i, getIntFromMapping(m));
	}
	m_qGLscatter->updateGL();
}


#define MAPPING_DATA_CHANGED(VAL) \
void WatchGeoTree::mappingDataChanged##VAL() \
{ \
    VertexBox *sendingVB = static_cast<VertexBox*>(sender()); \
    Mapping m = getMappingFromInt(cb##VAL->itemData(cb##VAL->currentIndex()).toInt()); \
    if (sendingVB) { \
        VertexBox *activeVB = NULL; \
        switch (m_dataSelection) { \
            case DATA_CURRENT: \
                activeVB = m_dataModel->getDataColumnCurrentData(m.index); \
                break; \
            case DATA_VERTEX: \
                activeVB = m_dataModel->getDataColumnVertexData(m.index); \
                break; \
        } \
        if (activeVB != sendingVB) { \
            disconnect(sendingVB, SIGNAL(dataChanged()), this, SLOT(mappingDataChanged##VAL())); \
            return; \
        } \
    } \
    RangeMapping rm = getRangeMappingFromInt(cbMap##VAL->itemData(cbMap##VAL->currentIndex()).toInt()); \
    switch (rm.range) { \
        case RANGE_MAP_DEFAULT: \
            tbMin##VAL->setText(QString::number(getDataMin(m.index))); \
            tbMax##VAL->setText(QString::number(getDataMax(m.index))); \
            break; \
        case RANGE_MAP_POSITIVE: \
            tbMin##VAL->setText(QString::number(MAX(getDataMin(m.index), 0.0))); \
            tbMax##VAL->setText(QString::number(MAX(getDataMax(m.index), 0.0))); \
            break; \
        case RANGE_MAP_NEGATIVE: \
            tbMin##VAL->setText(QString::number(MIN(getDataMin(m.index), 0.0))); \
            tbMax##VAL->setText(QString::number(MIN(getDataMax(m.index), 0.0))); \
            break; \
        case RANGE_MAP_ABSOLUTE: \
            tbMin##VAL->setText(QString::number(getDataAbsMin(m.index))); \
            tbMax##VAL->setText(QString::number(getDataAbsMax(m.index))); \
            break; \
    } \
    switch (m_dataSelection) { \
        case DATA_CURRENT: \
            updateDataCurrent(m_scatterData##VAL, &m_scatterDataCount##VAL, m_scatterDataStride##VAL, \
                    sendingVB, &m, &rm, dsMin##VAL->value(), dsMax##VAL->value()); \
            break; \
        case DATA_VERTEX: \
            updateDataVertex(m_scatterData##VAL, &m_scatterDataCount##VAL, m_scatterDataStride##VAL, \
                    sendingVB, &m, &rm, dsMin##VAL->value(), dsMax##VAL->value()); \
            break; \
    } \
    m_qGLscatter->updateGL(); \
}

MAPPING_DATA_CHANGED(Red)
MAPPING_DATA_CHANGED(Green)
MAPPING_DATA_CHANGED(Blue)
MAPPING_DATA_CHANGED(X)
MAPPING_DATA_CHANGED(Y)
MAPPING_DATA_CHANGED(Z)

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




