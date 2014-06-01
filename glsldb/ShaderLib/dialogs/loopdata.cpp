
#include "loopdata.h"
#include "colors.qt.h"
#include "data/dataBox.h"
#include "data/vertexBox.h"
#include "data/pixelBox.h"
#include "utils/dbgprint.h"
#include <QtGui/QColor>

LoopData::LoopData(ShaderMode mode, DataBox *condition, QObject *parent) :
		QObject(parent)
{
	vertexData = NULL;
	fragmentData = NULL;

	_model.setHorizontalHeaderItem(0, new QStandardItem("iteration"));
	_model.setHorizontalHeaderItem(1, new QStandardItem("active"));
	_model.setHorizontalHeaderItem(2, new QStandardItem("done"));
	_model.setHorizontalHeaderItem(3, new QStandardItem("out"));

	int size = condition->getSize();
	if (size) {
		initialCoverage = new bool[size];
		memcpy(initialCoverage, condition->getCoveragePointer(), size * sizeof(bool));
	}

	addLoopIteration(mode, condition, 0);
}

LoopData::~LoopData()
{
	delete[] initialCoverage;
	if (fragmentData)
		delete fragmentData;
	if (vertexData)
		delete vertexData;
}

void LoopData::addLoopIteration(ShaderMode mode, DataBox *condition, int iter)
{
	iteration = iter;
	DataBox *box = initializeBox(mode);
	box->copyFrom(condition);
	updateStatistic();
}

void LoopData::updateStatistic()
{
	int width = getWidth();
	int height = getHeight();
	int channels = fragmentData ? fragmentData->getChannels() : (vertexData ? 1 : 0);
	const float *condition = getActualCondition();
	const bool *coverage = getActualCoverage();

	if (!condition){
		dbgPrint(DBGLVL_ERROR, "LoopData features vertex and fragment data");
		exit(1);
	}

	totalValues = 0;
	activeValues = 0;
	doneValues = 0;
	outValues = 0;

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int offset = x * y;
			if (!initialCoverage[offset])
				continue;

			if (coverage[offset]) {
				if (condition[offset * channels] > 0.75f)
					activeValues++;
				else
					doneValues++;
			} else {
				outValues++;
			}
		}
	}

	totalValues = activeValues + doneValues + outValues;

	QList<QStandardItem*> rowData;
	rowData << new QStandardItem(QVariant(iteration).toString());
	rowData << new QStandardItem(QVariant(activeValues).toString());
	rowData << new QStandardItem(QVariant(doneValues).toString());
	rowData << new QStandardItem(QVariant(outValues).toString());
	_model.appendRow(rowData);
}

const bool* LoopData::getActualCoverage()
{
	if (fragmentData)
		return fragmentData->getCoveragePointer();
	if (vertexData)
		return vertexData->getCoveragePointer();
	return NULL;
}

const float* LoopData::getActualCondition()
{
	if (fragmentData)
		return static_cast<const float *>(fragmentData->getDataPointer());
	if (vertexData)
		return static_cast<const float *>(vertexData->getDataPointer());
	return NULL;
}

int LoopData::getWidth()
{
	if (fragmentData)
		return fragmentData->getWidth();
	if (vertexData)
		return 1;
	return 0;
}

int LoopData::getHeight()
{
	if (fragmentData)
		return fragmentData->getHeight();
	if (vertexData)
		return vertexData->getNumVertices();
	return 0;
}

QImage LoopData::getImage()
{
	int width = getWidth();
	int height = getHeight();
	QImage image(width, height, QImage::Format_RGB32);

	if (!fragmentData)
		return image;

	int channels = fragmentData->getChannels();
	const float *condition = getActualCondition();
	const bool *coverage = getActualCoverage();

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int offset = x * y;
			QColor color;
			if (initialCoverage[offset]) {
				/* initial data available */
				if (coverage[offset]) {
					/* actual data available */
					if (condition[offset * channels] > 0.75f)
						color = DBG_GREEN;
					else
						color = DBG_RED;
				} else {
					/* loop was left earlier at this pixel */
					color = DBG_ORANGE;
				}
			} else {
				/* no initial data, print checkerboard */
				color = DBG_COLOR_BY_POS(x, y);
			}

			image.setPixel(x, y, color.rgb());
		}
	}

	return image;
}

DataBox *LoopData::initializeBox(ShaderMode mode)
{
	DataBox *target = NULL;
	if (mode == smFragment) {
		if (!fragmentData)
			fragmentData = new PixelBox();
		target = fragmentData;
	} else if (mode == smVertex || mode == smGeometry) {
		if (vertexData)
			vertexData = new VertexBox();
		target = vertexData;
	}
	return target;
}

