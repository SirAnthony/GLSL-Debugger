
#include "dialogs/selection.h"
#include "ui_selectiondialog.h"
#include <QStackedWidget>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QTableView>
#include <QTreeView>
#include <QImage>

#include "colors.qt.h"
#include "models/vertextablemodel.h"
#include "models/geometrydatamodel.h"
#include "models/shvaritem.h"
#include "data/vertexBox.h"
#include "data/pixelBox.h"
#include "utils/dbgprint.h"


static inline QString to_string(int base, int total)
{
	if (!total)
		return "";
	return QString::number(base) + " (" +
			QString::number(base * 100 / total) + "%)";
}


SelectionDialog::SelectionDialog(ShaderMode mode, DataBox *box,
								 QList<ShVarItem *> &watch_items,
								 GeometryInfo &geometry,
								 bool else_branch, QWidget *parent) :
	QDialog(parent)
{
	/* Statistics */
	totalValues = 0;
	totalValues = 0;
	activeValues = 0;
	pathIf = 0;
	pathElse = 0;
	elseBranch = else_branch;
	ui->setupUi(this);

	// Connect
	connect(ui->pbSkip, SIGNAL(clicked()), this, SLOT(skipClick()));
	connect(ui->pbIf, SIGNAL(clicked()), this, SLOT(ifClick()));
	connect(ui->pbElse, SIGNAL(clicked()), this, SLOT(elseClick()));


	QWidget *widget;
	switch(mode) {
	case smVertex:
		widget = vertexWidget(dynamic_cast<VertexBox *>(box), watch_items);
		break;
	case smGeometry:
		widget = geometryWidget(dynamic_cast<VertexBox *>(box), watch_items, geometry);
		break;
	case smFragment:
		widget = fragmentWidget(dynamic_cast<PixelBox *>(box));
		break;
	default:
		widget = NULL;
		break;
	}

	QGridLayout *gridLayout = new QGridLayout(ui->fContent);
	gridLayout->setSpacing(0);
	gridLayout->setMargin(0);
	if (widget)
		gridLayout->addWidget(widget);

	/* Activate necessary buttons */
	if (else_branch) {
		ui->pbElse->setEnabled(true);
	} else {
		ui->pbElse->setEnabled(false);
		ui->lElseCount->setText(QString(""));
	}

	/* Show statistic data */
	displayStatistics();

	/* Disable options that no pixel is following */
	if (!pathIf)
		ui->pbIf->setEnabled(false);
	if (!pathElse)
		ui->pbElse->setEnabled(false);
}



void SelectionDialog::displayStatistics(void)
{
	ui->lActiveCount->setText(to_string(activeValues, totalValues));
	ui->lIfCount->setText(to_string(pathIf, activeValues));
	if (elseBranch)
		ui->lElseCount->setText(to_string(pathElse, activeValues));
	else
		ui->lElseCount->setText("");
}

int SelectionDialog::exec()
{
	/* Skip dialog if there is only one option for user input left */
	if (!activeValues)
		return SB_SKIP;

	if (!pathIf && !elseBranch)
		return SB_SKIP;

	return QDialog::exec();
}

void SelectionDialog::checkCounter()
{
	if (totalValues)
		return;

	/* Allow all possibilities */
	totalValues = 2;
	activeValues = 2;
	pathIf = 1;
	pathElse = 1;
}

bool SelectionDialog::calculateVertex(VertexBox *box)
{
	int verticles = box ? box->getNumVertices() : 0;
	totalValues = verticles;
	checkCounter();

	if (!box)
		return false;

	/* Display data and count */
	const float *condition = static_cast<const float*>(box->getDataPointer());
	const bool *coverage = box->getCoveragePointer();

	for (int i = 0; i < verticles; i++) {
		if (!coverage[i])
			continue;

		float cond = condition[i];
		if (cond > 0.75f)
			pathIf++;
		else if (0.25f < cond && cond < 0.75f)
			pathElse++;
		activeValues++;
	}

	return true;
}

void SelectionDialog::skipClick()
{
	done(SB_SKIP);
}

void SelectionDialog::ifClick()
{
	done(SB_IF);
}

void SelectionDialog::elseClick()
{
	done(SB_ELSE);
}

static void add_fragment_page(QStackedWidget *tgt, QImage &src, QString name)
{
	QWidget *widget = new QWidget();
	QVBoxLayout *layout = new QVBoxLayout();
	QScrollArea *scroll = new QScrollArea();
	QLabel *nameLabel = new QLabel(name);
	QLabel *pixmap = new QLabel();

	pixmap->setPixmap(QPixmap::fromImage(src));
	scroll->setBackgroundRole(QPalette::Dark);
	scroll->setWidget(pixmap);

	// Put items
	layout->addWidget(nameLabel);
	layout->addWidget(scroll);
	widget->setLayout(layout);
	tgt->addWidget(widget);
}

QWidget *SelectionDialog::vertexWidget(VertexBox *box, QList<ShVarItem*> &watch_items)
{
	QTableView *table = new QTableView(this);
	table->setAlternatingRowColors(true);

	if (!calculateVertex(box))
		return table;

	VertexTableModel *model = new VertexTableModel();
	foreach(ShVarItem* item, watch_items) {
		QString name = item->data(DF_FULLNAME).toString();
		VertexBox *vbox = static_cast<VertexBox *>(
					item->data(DF_DATA_VERTEXBOX).value<void *>());
		model->addVertexBox(vbox, name);
	}
	model->setCondition(box);
	table->setModel(model);

	if (watch_items.count() != 0)
		table->setColumnHidden(0, true);

	return table;
}

QWidget *SelectionDialog::geometryWidget(VertexBox *box, QList<ShVarItem *> &watch_items,
										 GeometryInfo &geometry)
{
	QTreeView *tree = new QTreeView(this);
	tree->setAlternatingRowColors(true);

	if (!calculateVertex(box))
		return tree;

	GeometryDataModel *model = new GeometryDataModel(geometry.primitiveMode,
					geometry.outputType, geometry.map, geometry.count, box, NULL);
	foreach(ShVarItem* item, watch_items) {
		QString name = item->data(DF_FULLNAME).toString();
		VertexBox *vbox = static_cast<VertexBox *>(
					item->data(DF_DATA_VERTEXBOX).value<void *>());
		VertexBox *cbox = static_cast<VertexBox *>(
					item->data(DF_DATA_GEOMETRYBOX).value<void *>());
		model->addData(cbox, vbox, name);
	}

	tree->setModel(model);
	if (watch_items.count() != 0)
		tree->setColumnHidden(1, true);

	return tree;
}

QWidget * SelectionDialog::fragmentWidget(PixelBox *box)
{
	QStackedWidget *holder = new QStackedWidget(this);

	if (box) {
		int channels = box->getChannels();
		int width = box->getWidth();
		int height = box->getHeight();
		double v[channels];
		QImage images[channels];

		totalValues = width * height * channels;
		checkCounter();

		for (int i = 0; i < channels; ++i)
			images[i] = QImage(width, height, QImage::Format_RGB32);

		for (int y = 0; y < height; y++) {
			for (int x = 0; x < width; x++) {
				bool has_values = box->getDataValue(x, y, v);
				for (int c = 0; c < channels; ++c) {
					QColor color;
					if (has_values) {
						if (v[c] > 0.75) {
							pathIf++;
							color = DBG_GREEN;
						} else if (0.25 < v[c] && v[c] < 0.75) {
							pathElse++;
							color = DBG_RED;
						}
						activeValues++;
					} else {
						/* no data, print checkerboard */
						if (((x / 8) % 2) == ((y / 8) % 2))
							color = QColor(255, 255, 255);
						else
							color = QColor(204, 204, 204);
					}
					images[c].setPixel(x, y, color.rgb());
				}
			}
		}

		for (int c = 0; c < channels; ++c)
			add_fragment_page(holder, images[c],
							  "Channel " + QString::number(c));
	} else {
		checkCounter();
		QImage image(512, 512, QImage::Format_RGB32);
		add_fragment_page(holder, image, "No data");
	}

	return holder;
}

