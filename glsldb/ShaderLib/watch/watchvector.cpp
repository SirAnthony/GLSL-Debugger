
#include "watchvector.h"
#include "ui_watchvector.h"
#include <QFrame>
#include <QScrollArea>
#include <QScrollBar>
#include <QMessageBox>
#include <QDir>
#include <QFileDialog>
#include <QImage>
#include <QImageWriter>
#include <QWorkspace>
#include <math.h>

#include "shwindowmanager.h"
#include "mappings.h"
#include "imageview.h"
#include "data/pixelBox.h"
#include "utils/dbgprint.h"


WatchVector::WatchVector(QWidget *parent) :
		WatchView(parent),
		widgets { ui->MappingRed, ui->MappingGreen, ui->MappingBlue }
{
	ui->setupUi(this);
	ui->fMapping->setVisible(false);
	type = ShWindowManager::wtFragment;

	ui->scrollArea->setBackgroundRole(QPalette::Dark);
	viewCenter[0] = -1;
	viewCenter[1] = -1;

	/* Initialize attachments */
	for (int i = 0; i < MAX_ATTACHMENTS; i++)
		vectorData[i] = NULL;

	/* Initialize member variables */
	needsUpdate = true;
	activeMappings = 0;

	for (int i = 0; i < WV_WIDGETS_COUNT; ++i)
		connectWidget(widgets[i]);

	connect(ui->imageView, SIGNAL(mousePosChanged(int, int)), this,
			SLOT(setMousePos(int, int)));
	connect(ui->imageView, SIGNAL(picked(int, int)), this,
			SIGNAL(selectionChanged(int,int)));
	connect(ui->imageView, SIGNAL(viewCenterChanged(int, int)), this,
			SLOT(newViewCenter(int, int)));
	connect(ui->imageView, SIGNAL(updateFocus(int)), this,
			SLOT(updateFocus(Qt::FocusReason)));

	connect(ui->tbSaveImage, SIGNAL(clicked()), this, SLOT(saveImage()));
	ui->imageView->setFocusPolicy(Qt::StrongFocus);
	ui->imageView->setFocus();
}

void WatchVector::attachData(DataBox *dbox, QString &name)
{
	PixelBox *box = dynamic_cast<PixelBox*>(dbox);
	if (!box || !freeMappingsCount() || dataIndex(box) >= 0)
		return;

	/* Fill empty slot with attachment */
	int idx = getFreeMapping();
	vectorData[idx] = box;
	activeMappings++;

	connect(box, SIGNAL(dataChanged()), this, SLOT(updateData()));
	connect(box, SIGNAL(dataDeleted()), this, SLOT(detachData()));
	connect(ui->imageView, SIGNAL(minMaxAreaChanged(const QRect&)), box,
			SLOT(setMinMaxArea(const QRect&)));
	connect(box, SIGNAL(minMaxAreaChanged()), this, SLOT(updateGUI()));

	for (int i = 0; i < WV_WIDGETS_COUNT; ++i) {
		connect(ui->imageView, SIGNAL(setMappingBounds()), widgets[i],
				SLOT(updateBoundaries()));

		widgets[i]->addOption(idx, name);
		widgets[i]->updateData();
	}
}

void WatchVector::connectActions(struct MenuActions *actions)
{
	connect(actions->zoom, SIGNAL(triggered()), this, SLOT(setZoomMode()));
	connect(actions->selectPixel, SIGNAL(triggered()), this, SLOT(setPickMode()));
	connect(actions->minMaxLens, SIGNAL(triggered()), this, SLOT(setMinMaxMode()));

	QList<QAction*> group;
	group << actions->zoom;
	group << actions->selectPixel;
	group << actions->minMaxLens;
	foreach(QAction* action, group) {
		action->setEnabled(true);
		if(action->isChecked())
			action->trigger();
	}
}

int WatchVector::freeMappingsCount()
{
	int count = MAX_ATTACHMENTS - activeMappings;
	return (count > 0) ? count : 0;
}

int WatchVector::getFreeMapping()
{
	for (int i = 0; i < MAX_ATTACHMENTS; i++) {
		if (vectorData[i] == NULL)
			return i;
	}
	return -1;
}

void WatchVector::setBoundaries(int index, double *min, double *max, bool absolute)
{
	PixelBox *pb = vectorData[index];
	if (absolute) {
		*min = pb->getAbsMin();
		*max = pb->getAbsMax();
	} else {
		*min = pb->getMin();
		*max = pb->getMax();
	}
}

void WatchVector::setDataBox(int idx, DataBox **box)
{
	*box = vectorData[idx];
}

void WatchVector::saveImage()
{
	static QStringList history;
	static QDir directory = QDir::current();
	static QString filters = "Portable Network Graphics (*.png);;"
			"Windows Bitmap (*.bmp);;" "Portable Pixmap (*.ppm);;"
			"Joint Photographic Experts Group (*.jpg, *.jepg);;"
			"Tagged Image File Format (*.tif, *.tiff);;"
			"X11 Bitmap (*.xbm, *.xpm)";

	QFileDialog dialog(this, QString("Save image"), directory.path(), filters);
	dialog.setAcceptMode(QFileDialog::AcceptSave);
	dialog.setFileMode(QFileDialog::AnyFile);
	if (!history.isEmpty())
		dialog.setHistory(history);

	if (dialog.exec()) {
		QStringList files = dialog.selectedFiles();
		if (!files.isEmpty()) {
			QString selected = files[0];
			QFileInfo fileInfo(selected);
			QImage *img = drawNewImage(false);
			if (!(img->save(selected)))
				dbgPrint(DBGLVL_ERROR, "Cannot save image file.");
			delete img;
		}
	}

	history = dialog.history();
	directory = dialog.directory();
}

static void setImageChannel(enum ImageChannels channel, QImage *image,
							const bool *pCover, int value)
{
	int width = image->width();
	int height = image->height();
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			QColor c(image->pixel(x, y));
			int new_color = value;
			if (!pCover[y * width + x])
				new_color = ((x / 8) % 2) == ((y / 8) % 2) ? 255 : 204;

			switch (channel) {
			case icRed:
				c.setRed(new_color);
				break;
			case icGreen:
				c.setGreen(new_color);
				break;
			case icBlue:
				c.setBlue(new_color);
				break;
			}

			image->setPixel(x, y, c.rgb());
		}
	}
}


QImage* WatchVector::drawNewImage(bool alpha)
{
	int width, height;
	const bool *pCover = NULL;

	width = height = 0;
	for (int i = 0; i < MAX_ATTACHMENTS; i++) {
		if (!vectorData[i])
			continue;
		/* Search for covermap */
		// Wat?
		pCover = vectorData[i]->getCoveragePointer();

		/* Check if attached data is valid */
		if (!width)
			width = vectorData[i]->getWidth();
		else if (width != vectorData[i]->getWidth())
			dbgPrint(DBGLVL_ERROR, "WatchVector is composed of differently sized float boxes");

		if (!height)
			height = vectorData[i]->getHeight();
		else if (height != vectorData[i]->getHeight())
			dbgPrint(DBGLVL_ERROR, "WatchVector is composed of differently sized float boxes");
	}

	const ImageChannels channels[3] = { icRed, icGreen, icBlue };
	QImage *image = new QImage(width, height,
			alpha ? QImage::Format_ARGB32 : QImage::Format_RGB32);


	float minmax[2];
	for (int i = 0; i < WV_WIDGETS_COUNT; ++i) {
		ShMappingWidget* w = widgets[i];
		Mapping m = getMappingFromInt(w->currentValData());
		if (m.type == MAP_TYPE_VAR && vectorData[m.index]) {
			RangeMapping rm = getRangeMappingFromInt(w->currentMapData());
			w->minMax(minmax);
			vectorData[m.index]->setByteImageChannel(channels[i], image, rm.range,
													 minmax, alpha);
		} else {
			setImageChannel(channels[i], image, pCover, m.type == MAP_TYPE_WHITE ? 255 : 0);
		}
	}

	return image;
}

void WatchVector::updateView(bool force)
{
	emit updateGUI();
	if (!needsUpdate && !force)
		return;

	QImage *image = drawNewImage(false);
	int scrollPosX = ui->scrollArea->horizontalScrollBar()->value();
	int scrollPosY = ui->scrollArea->verticalScrollBar()->value();

	setUpdatesEnabled(false);
	ui->imageView->resize(image->width(), image->height());
	ui->imageView->setImage(*image);
	ui->scrollArea->horizontalScrollBar()->setValue(scrollPosX);
	ui->scrollArea->verticalScrollBar()->setValue(scrollPosY);
	setUpdatesEnabled(true);
	update();

	delete image;

	needsUpdate = false;
}

QAbstractItemModel *WatchVector::model()
{
	return NULL;
}

int WatchVector::dataIndex(PixelBox *f)
{
	for (int i = 0; i < MAX_ATTACHMENTS; i++) {
		if (vectorData[i] == f)
			return i;
	}
	return -1;
}

void WatchVector::detachData()
{
	PixelBox *f = static_cast<PixelBox*>(sender());
	int idx = dataIndex(f);

	if (idx >= 0) {
		activeMappings--;
		vectorData[idx] = NULL;
		needsUpdate = true;
		for (int i = 0; i < WV_WIDGETS_COUNT; ++i)
			widgets[i]->delOption(idx);
	}

	if (!activeMappings)
		closeView();
}

void WatchVector::setMousePos(int x, int y)
{
	if (x >= 0 && y >= 0) {
		PixelBox *activeChannels[3];
		QVariant values[3];
		bool active[3];
		getActiveChannels(activeChannels);
		for (int i = 0; i < 3; i++) {
			if (activeChannels[i]) {
				active[i] = activeChannels[i]->getDataValue(x, y, &values[i]);
			} else {
				active[i] = false;
				values[i] = 0.0;
			}
		}
		emit mouseOverValuesChanged(x, y, active, values);
	} else {
		emit mouseOverValuesChanged(-1, -1, NULL, NULL);
	}
}

void WatchVector::setZoomMode()
{
	ui->imageView->setMouseMode(ImageView::MM_ZOOM);
}

void WatchVector::setPickMode()
{
	ui->imageView->setMouseMode(ImageView::MM_PICK);
}

void WatchVector::setMinMaxMode()
{
	ui->imageView->setMouseMode(ImageView::MM_MINMAX);
}

void WatchVector::updateFocus(Qt::FocusReason focus)
{
	QWidget* widget = dynamic_cast<QWidget*>(sender());
	if (widget && this->isActiveWindow())
		widget->setFocus(focus);
}

void WatchVector::newViewCenter(int x, int y)
{
	viewCenter[0] = x;
	viewCenter[1] = y;
	ui->scrollArea->ensureVisible(viewCenter[0], viewCenter[1],
		ui->scrollArea->width() / 2 - 1, ui->scrollArea->height() / 2 - 1);
}

void WatchVector::updateGUI()
{
	QStringList title;
	for (int i = 0; i < 3; i++)
		title << widgets[i]->asText();
	setWindowTitle(title.join(", "));
}

void WatchVector::updateData(int, int, float, float)
{
	needsUpdate = true;
}

void WatchVector::getActiveChannels(PixelBox *channels[3])
{
	for (int i = 0; i < WV_WIDGETS_COUNT; i++) {
		Mapping m = getMappingFromInt(widgets[i]->currentValData());
		if (m.type != MAP_TYPE_BLACK && m.type != MAP_TYPE_WHITE)
			channels[i] = vectorData[m.index];
		else
			channels[i] = NULL;
	}
}

