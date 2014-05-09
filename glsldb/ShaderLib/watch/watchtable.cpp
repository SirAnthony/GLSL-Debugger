
#include "mappings.h"
#include "watchtable.h"
#include "ui_watchtable.h"
#include "vertexTableModel.qt.h"
#include "glscatter.h"
#include "shmappingwidget.h"
#include "dbgprint.h"

static void clearData(float *data, int count, int dataStride, float clearValue)
{
	for (int i = 0; i < count; i++) {
		*data = clearValue;
		data += dataStride;
	}
}

WatchTable::WatchTable(QWidget *parent) :
		WatchView(parent)
{
	ui->setupUi(this);
	ui->fMapping->setVisible(false);
	m_vertexTable = new VertexTableModel(this);
	m_filterProxy = new VertexTableSortFilterProxyModel(this);	
	m_filterProxy->setSourceModel(m_vertexTable);
	m_filterProxy->setDynamicSortFilter(true);	
	connect(tbHideInactive, SIGNAL(toggled(bool)), m_filterProxy, SLOT(setHideInactive(bool)));
	connect(m_vertexTable, SIGNAL(dataDeleted(int)), this, SLOT(detachData(int)));
	connect(m_vertexTable, SIGNAL(empty()), this, SLOT(closeView()));
	tvVertices->setModel(m_filterProxy);

	connect(tvVertices, SIGNAL(doubleClicked(const QModelIndex &)), this,
			SLOT(newSelection(const QModelIndex &)));

	scatterPositions = NULL;
	scatterColorsAndSizes = NULL;
	maxScatterDataElements = 0;
	scatterDataElements = 0;
	widgets = {
		ui->MappingX, ui->MappingY, ui->MappingZ,
		ui->MappingRed, ui->MappingGreen, ui->MappingBlue
	};

	ui->glScatter->setData(NULL, NULL, 0);
	updatePointSize(ui->slPointSize->value());
	updateGUI();
}

WatchTable::~WatchTable()
{
	ui->glScatter->setData(NULL, NULL, 0);
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
	float *dd = data;
	float *ds = srcData->getDataPointer();
	bool *dc = srcData->getCoveragePointer();
	bool *dm = srcData->getDataMapPointer();

	max = fabs(max - min) > FLT_EPSILON ? max : 1.0f + min;
	clearData(data, srcData->getNumVertices(), dataStride, 0.0f);

	*count = 0;
	for (int i = 0; i < srcData->getNumVertices(); i++) {
		if ((dc && !*dc) || (dm && !*dm)) {
		} else {
			*dd = getMappedValueF(*ds, range, min, max);
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

	ui->glScatter->setData(scatterPositions, scatterColorsAndSizes,
			scatterDataElements);
}

void WatchTable::attachVpData(VertexBox *vb, QString name)
{
	if (!vb)
		return;

	if (model->addVertexBox(vb, name)) {
		if (!scatterPositions) {
			maxScatterDataElements = vb->getNumVertices();
			scatterDataElements = 0;
			scatterPositions = new float[3 * maxScatterDataElements];
			scatterColorsAndSizes = new float[3 * maxScatterDataElements];
			for (int i = 0; i < WT_WIDGETS_COUNT; ++i) {
				scatterData[i] = scatterPositions + (i % 3);
				clearData(scatterData[i], maxScatterDataElements,
						  scatterDataStride[i], 0.0f);
				scatterDataCount[i] = 0;
			}
		}
		int idx = model->columnCount() - 1;
		QString name = model->getDataColumnName(idx);
		for (int i = 0; i < WT_WIDGETS_COUNT; ++i)
			widgets[i]->addOption(idx, name);

		updateGUI();
	}
}

void WatchTable::updateData(int index, int range, float min, float max)
{
	ShMappingWidget *sndr = static_cast<ShMappingWidget*>(sender());
	int idx = WT_WIDGETS_COUNT;
	while (--idx > 0)
		if (sndr = widgets[idx])
			break;

	if (idx < 0)
		return;

	updateDataCurrent(scatterData[idx], &scatterDataCount[idx],
					  scatterDataStride[idx], model->getDataColumn(index),
					  range, min, max);
}

void WatchTable::detachData(int idx)
{
	dbgPrint(DBGLVL_DEBUG, "WatchTable::detachData: idx=%i\n", idx);
	for (int i = 0; i < WT_WIDGETS_COUNT; ++i)
		widgets[i]->delOption(idx);
}

void WatchTable::clearData()
{
	ShMappingWidget *sndr = static_cast<ShMappingWidget*>(sender());
	int idx = WT_WIDGETS_COUNT;
	while (--idx > 0)
		if (sndr = widgets[idx])
			break;

	if (idx < 0)
		return;

	clearData(scatterData[idx], maxScatterDataElements,
			  scatterDataStride[idx], 0.0f);
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

void WatchTable::updateGUI()
{
	QString title("");
	int columns = model->columnCount();
	for (int i = 0; i < columns; i++) {
		if (i > 0)
			title += ", ";
		title += model->headerData(i, Qt::Horizontal).toString();
	}
	setWindowTitle(title);
}

void WatchTable::updateView(bool force)
{
	UNUSED_ARG(force)
	dbgPrint(DBGLVL_DEBUG, "WatchTable updateView\n");
	model->updateData();
	qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
	update();
	qApp->processEvents(QEventLoop::ExcludeUserInputEvents);
}

void WatchTable::closeView()
{
	hide();
	deleteLater();
}

