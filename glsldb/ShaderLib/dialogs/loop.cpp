
#include "loop.h"
#include "ui_loopdialog.h"
#include <QtGui/QHeaderView>
#include <QtGui/QVBoxLayout>

#include <QImage>
#include <QScrollArea>
#include <QTableView>
#include <QTreeView>

#include "colors.qt.h"
#include "models/shvaritem.h"
#include "models/geometrydatamodel.h"
#include "models/vertextablemodel.h"
#include "utils/dbgprint.h"

#define STAT_ROW_HEIGTH 22
#define CURVE_COLORS 3

static QColor curves[CURVE_COLORS] = {
	DBG_GREEN, DBG_RED, DBG_ORANGE
};


static void hide_column(ShaderMode mode, QAbstractItemView *view)
{
	if (mode == smGeometry) {
		QTreeView *nview = dynamic_cast<QTreeView *>(view);
		nview->setColumnHidden(1, true);
	} else if (mode == smVertex) {
		QTableView *nview = dynamic_cast<QTableView *>(view);
		nview->setColumnHidden(1, true);
	}
}

/* Fragment Shader */
LoopDialog::LoopDialog(ShaderMode mode, LoopData *data, QSet<ShVarItem *> &watchItems,
					   GeometryInfo &geometry, QWidget *parent) :
		QDialog(parent), ui(new Ui::ShLoop)
{
	loopData = data;
	ui->setupUi(this);
	setupGui();

	if (!data)
		return;

	QLayout* container = ui->fContent->layout();

	/* Main GUI window */
	if (data->isFragmentLoop()) {
		QScrollArea* scroll = new QScrollArea();
		scroll->setBackgroundRole(QPalette::Dark);
		container->addWidget(scroll);

		image = new QLabel();
		image->setPixmap(QPixmap::fromImage(data->getImage()));
		scroll->setWidget(image);
	} else if (!data->isVertexLoop()) {
		return;
	}

	QAbstractItemView *view = NULL;
	ShTableDataModel *tmodel = NULL;
	QAbstractItemModel *imodel = NULL;
	if (mode == smGeometry) {
		view = new QTreeView(this);
		GeometryDataModel *model = new GeometryDataModel(geometry,
														 data->getActualVertexBox(),
														 data->getInitialCoverage());
		tmodel = model;
		imodel = model;
	} else if (mode == smVertex) {
		view = new QTableView(this);
		VertexTableModel *model = new VertexTableModel(data->getActualVertexBox(),
													   data->getInitialCoverage());
		tmodel = model;
		imodel = model;
	}

	foreach (ShVarItem *item, watchItems)
		tmodel->addItem(item);

	view->setAlternatingRowColors(true);
	view->setModel(imodel);
	container->addWidget(view);
	if (watchItems.count() > 0)
		hide_column(mode, view);
}

int LoopDialog::exec()
{
	if (!activeValues)
		return SA_BREAK;
	return QDialog::exec();
}

void LoopDialog::stateChanged(int curve, int state)
{
	if (state == Qt::Unchecked)
		ui->fCurvesDisplay->delMapping(curve);
	else if (state == Qt::Checked)
		ui->fCurvesDisplay->addMapping(curve, curves[curve]);
}

void LoopDialog::calculateStatistics()
{
	/* Initialization */
	ui->pbBreak->setEnabled(false);
	ui->pbStatistics->setEnabled(false);
	ui->pbJump->setEnabled(false);
	ui->pbNext->setEnabled(false);
	ui->pbProgress->setValue(loopData->getIteration());
	ui->fProgress->setVisible(true);
	processStopped = false;
	ui->tvStatTable->setUpdatesEnabled(false);
	ui->fCurvesDisplay->setUpdatesEnabled(false);

	/* Process loop */
	while ((loopData->getIteration() < MAX_LOOP_ITERATIONS - 1)
			&& (loopData->getActive() > 0) && !processStopped) {
		emit doStep(DBG_BH_LOOP_NEXT_ITER, false, true);
		ui->pbProgress->setValue(loopData->getIteration());
		updateIterationStatistics();
		qApp->processEvents(QEventLoop::AllEvents);
	}

	ui->tvStatTable->setUpdatesEnabled(true);
	ui->fCurvesDisplay->setUpdatesEnabled(true);

	emit doStep(DBG_BH_LOOP_NEXT_ITER, true, true);
	if (loopData->isFragmentLoop())
		image->setPixmap(QPixmap::fromImage(loopData->getImage()));
	qApp->processEvents(QEventLoop::AllEvents);

	/* Restore valid GUI state */
	bool active = (loopData->getIteration() < MAX_LOOP_ITERATIONS &&
				   loopData->getActive() > 0);

	ui->pbBreak->setEnabled(true);
	ui->pbStatistics->setEnabled(active);
	ui->pbJump->setEnabled(active);
	ui->pbNext->setEnabled(active);
	ui->fProgress->setVisible(false);
}

void LoopDialog::nextIteration()
{
	done(SA_NEXT);
}

void LoopDialog::doJump()
{
	int iter = loopData->getIteration();
	ui->pbBreak->setEnabled(false);
	ui->pbStatistics->setEnabled(false);
	ui->pbNext->setEnabled(false);

	if (ui->fJump->isVisible()) {
		ui->pbJump->setEnabled(false);
		int position = ui->hsJump->sliderPosition();
		for (int i = iter; i < position; i++)
			emit doStep(DBG_BH_LOOP_NEXT_ITER, false, false);
		done(SA_JUMP);
	} else {
		ui->pbJump->setEnabled(true);
		ui->hsJump->setMinimum(iter + 1);
		ui->hsJump->setMaximum(MAX_LOOP_ITERATIONS);
		ui->hsJump->setSliderPosition(iter + 1);
		ui->fJump->setVisible(true);
	}
}

void LoopDialog::doBreak()
{
	done(SA_BREAK);
}

void LoopDialog::stopProgress()
{
	processStopped = true;
}

void LoopDialog::stopJump()
{
	ui->fJump->setVisible(false);
	ui->pbBreak->setEnabled(true);
	if (loopData->getIteration() < MAX_LOOP_ITERATIONS) {
		ui->pbStatistics->setEnabled(true);
		ui->pbJump->setEnabled(true);
		ui->pbNext->setEnabled(true);
	}
}

void LoopDialog::on_cbActive_stateChanged(int state)
{
	emit stateChanged(1, state);
}

void LoopDialog::on_cbDone_stateChanged(int state)
{
	emit stateChanged(2, state);
}

void LoopDialog::on_cbOut_stateChanged(int state)
{
	emit stateChanged(3, state);
}

void LoopDialog::reorganizeLoopTable(const QModelIndex &, int, int)
{
	QAbstractItemModel *data = ui->tvStatTable->model();
	for (int i = 0; i < data->columnCount(); i++)
		ui->tvStatTable->resizeColumnToContents(i);
	for (int i = 0; i < data->rowCount(); i++)
		ui->tvStatTable->setRowHeight(i, STAT_ROW_HEIGTH);
}

void LoopDialog::setupGui(void)
{
	/* Curves */
	ui->fCurvesDisplay->setModel(loopData->getModel());
	ui->fCurvesDisplay->setBase(0);
	for (int i = 0; i < CURVE_COLORS; ++i)
		ui->fCurvesDisplay->addMapping(i, curves[i]);

	ui->pbProgress->setMaximum(MAX_LOOP_ITERATIONS);
	ui->fProgress->setVisible(false);
	ui->fJump->setVisible(false);

	if (loopData->getIteration() < MAX_LOOP_ITERATIONS) {
		ui->pbStatistics->setEnabled(true);
		ui->pbJump->setEnabled(true);
		ui->pbNext->setEnabled(true);
	} else {
		ui->pbStatistics->setEnabled(false);
		ui->pbJump->setEnabled(false);
		ui->pbNext->setEnabled(false);
	}

	/* Statistic text */
	updateIterationStatistics();

	/* Statistics */
	QStandardItemModel *data = loopData->getModel();
	ui->tvStatTable->setModel(data);
	ui->tvStatTable->verticalHeader()->hide();
	reorganizeLoopTable(QModelIndex(), 0, 0);
	connect(data, SIGNAL(rowsInserted(const QModelIndex&, int, int)),
			this, SLOT(reorganizeLoopTable(const QModelIndex&, int, int)));

	/* Store data for possible skip */
	activeValues = loopData->getActive();
}

void LoopDialog::updateIterationStatistics()
{
	/* Iteration Statistics */
	int total = loopData->getTotal();
	int active = loopData->getActive();
	int done = loopData->getDone();
	int out = loopData->getOut();
	int size = loopData->getWidth() * loopData->getHeight();

	ui->lTotalCount->setText(to_string(total, size, QString::number(total)));
	ui->lActiveCount->setText(to_string(active, total));
	ui->lDoneCount->setText(to_string(done, total));
	ui->lOutCount->setText(to_string(out, total));
}
