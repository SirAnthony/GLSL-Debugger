
#include "mappings.h"
#include "watchtable.h"
#include "ui_watchtable.h"
#include "models/vertextablemodel.h"
#include "glscatter.h"
#include "shmappingwidget.h"
#include "utils/dbgprint.h"
#include <cfloat>

#define ELEMENTS_PER_VERTEX 3


WatchTable::WatchTable(QWidget *parent) :
	WatchView(parent),
	widgets {
		ui->MappingX, ui->MappingY, ui->MappingZ,
		ui->MappingRed, ui->MappingGreen, ui->MappingBlue
	}
{
	ui->setupUi(this);
	ui->fMapping->setVisible(false);
	_model = new VertexTableModel(this);
	_modelFilter = new VertexTableSortFilterProxyModel(this);
	_modelFilter->setSourceModel(_model);
	_modelFilter->setDynamicSortFilter(true);
	connect(ui->tbHideInactive, SIGNAL(toggled(bool)), _modelFilter, SLOT(setHideInactive(bool)));
	connect(_model, SIGNAL(dataDeleted(int)), this, SLOT(detachData(int)));
	connect(_model, SIGNAL(empty()), this, SLOT(closeView()));
	ui->tvVertices->setModel(_model);

	connect(ui->tvVertices, SIGNAL(doubleClicked(const QModelIndex &)), this,
			SLOT(newSelection(const QModelIndex &)));

	scatterPositions = NULL;
	scatterColorsAndSizes = NULL;
	maxScatterDataElements = 0;
	scatterDataElements = 0;
	ui->glScatter->setData(NULL, NULL, 0, 0);
	updatePointSize(ui->slPointSize->value());
	updateGUI();
}

WatchTable::~WatchTable()
{
	ui->glScatter->setData(NULL, NULL, 0, 0);
	delete[] scatterPositions;
	delete[] scatterColorsAndSizes;
}

bool WatchTable::countsAllZero()
{
	for (int i = 0; i < WT_WIDGETS_COUNT; ++i)
		if (scatterDataCount[i] != 0)
			return false;
	return true;
}

void WatchTable::updateDataCurrent(float *data, int *count, int dataStride,
		VertexBox *srcData, RangeMap range, float min, float max)
{	
	float *pData = data;
	const float *pSourceData = srcData->getDataPointer();
	const bool *pSourceCov = srcData->getCoveragePointer();
	const bool *pSourceMap = srcData->getDataMapPointer();
	int verticles = srcData->getNumVertices();

	max = fabs(max - min) > FLT_EPSILON ? max : 1.0f + min;
	clearDataArray(data, srcData->getNumVertices(), dataStride, 0.0f);

	int total = 0;
	for (int i = 0; i < verticles; i++) {
		if ((pSourceCov && !pSourceCov[i]) || (pSourceMap && !pSourceMap[i]))
			continue;
		*pData = getMappedValueF(pSourceData[i], range, min, max);
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

void WatchTable::updateView(bool force)
{
	UNUSED_ARG(force)
	dbgPrint(DBGLVL_DEBUG, "WatchTable updateView\n");
	_model->updateData();
	qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
	update();
	qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
}

void WatchTable::attachVpData(VertexBox *vb, QString name)
{
	if (!vb)
		return;

	if (_model->addVertexBox(vb, name)) {
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
		int idx = _model->columnCount() - 1;
		QString name = _model->getDataColumnName(idx);
		for (int i = 0; i < WT_WIDGETS_COUNT; ++i)
			widgets[i]->addOption(idx, name);

		updateGUI();
	}
}

QAbstractItemModel* WatchTable::model()
{
	return _model;
}

void WatchTable::connectDataBox(int index)
{
	ShMappingWidget* widget = dynamic_cast<ShMappingWidget*>(sender());
	VertexBox *vb = _model->getDataColumn(index);
	connect(vb, SIGNAL(dataChanged()), widget, SLOT(mappingDataChanged()));
}

void WatchTable::updateData(int index, int range, float min, float max)
{
	ShMappingWidget *sndr = static_cast<ShMappingWidget*>(sender());
	int idx = WT_WIDGETS_COUNT;
	while (--idx > 0)
		if (sndr == widgets[idx])
			break;

	if (idx < 0)
		return;

	updateDataCurrent(scatterData[idx], &scatterDataCount[idx],
					  scatterDataStride[idx], _model->getDataColumn(index),
					  static_cast<RangeMap>(range), min, max);
}

void WatchTable::clearData()
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

void WatchTable::setBoundaries(int index, double *min, double *max, bool absolute)
{
	VertexBox *vb = _model->getDataColumn(index);
	if (absolute) {
		*min = vb->getAbsMin();
		*max = vb->getAbsMax();
	} else {
		*min = vb->getMin();
		*max = vb->getMax();
	}
}

void WatchTable::setDataBox(int index, DataBox **box)
{
	*box = _model->getDataColumn(index);
}

void WatchTable::detachData(int idx)
{
	dbgPrint(DBGLVL_DEBUG, "WatchTable::detachData: idx=%i\n", idx);
	for (int i = 0; i < WT_WIDGETS_COUNT; ++i)
		widgets[i]->delOption(idx);
}

void WatchTable::newSelection(const QModelIndex & index)
{
	emit selectionChanged(index.row());
}

void WatchTable::updatePointSize(int value)
{
	float newValue = 0.03 * (float) value / (float) ui->slPointSize->maximum();
	ui->lbPointSize->setText(QString::number(newValue));
	ui->glScatter->setPointSize(newValue);
	ui->glScatter->updateGL();
}

void WatchTable::closeView()
{
	hide();
	deleteLater();
}


