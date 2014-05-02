
#include <float.h>
#include <limits.h>
#include <math.h>
#include "pixelBox.h"
#include "dbgprint.h"
#include <QColor>
#include <QVariant>


template<float> double get_min() { return -FLT_MAX; }
template<float> double get_max() { return FLT_MAX; }
template<int> double get_min() { return INT_MIN; }
template<int> double get_max() { return INT_MAX; }
template<unsigned int> double get_min() { return 0; }
template<unsigned int> double get_max() { return UINT_MAX; }

PixelBox::PixelBox(QObject *parent) :
		DataBox(parent)
{
	width = 0;
	height = 0;
	channel = 0;	
	coverage = NULL;
	boxData = NULL;
	boxDataMap = NULL;
	boxDataMin = NULL;
	boxDataMax = NULL;
	boxDataMinAbs = NULL;
	boxDataMaxAbs = NULL;
}

PixelBox::PixelBox(PixelBox *src) :
		DataBox(src->parent())
{
	width = src->width;
	height = src->height;
	channel = src->channel;
	coverage = src->coverage;
	typeSize = src->typeSize;
	minVal = src->minVal;
	maxVal = src->maxVal;

	size_t depth = typeSize * channel;
	boxData = malloc(width * height * depth);
	boxDataMap = malloc(width * height * sizeof(bool));
	memcpy(boxData, src->boxData, width * height * depth);
	memcpy(boxDataMap, src->boxDataMap, width * height * sizeof(bool));

	boxDataMin = malloc(depth);
	boxDataMax = malloc(depth);
	boxDataMinAbs = malloc(depth);
	boxDataMaxAbs = malloc(depth);
	memcpy(boxDataMin, src->boxDataMin, depth);
	memcpy(boxDataMax, src->boxDataMax, depth);
	memcpy(boxDataMinAbs, src->boxDataMinAbs, depth);
	memcpy(boxDataMaxAbs, src->boxDataMaxAbs, depth);
}

PixelBox::~PixelBox()
{
	clear();
}

void PixelBox::setMinMaxArea(const QRect& minMaxArea)
{
	this->minMaxArea = minMaxArea;
	emit minMaxAreaChanged();
}

bool* PixelBox::getCoverageFromData(int *_activePixels)
{
	bool *cov = new bool[width * height];
	int active = 0;

	for (int i = 0; i < width * height; i++) {
		int offset = i * typeSize + channel * typeSize;
		if (boxData[offset] > 0.5f) {
			cov[i] = true;
			active++;
		} else {
			cov[i] = false;
		}
	}
	if (_activePixels)
		*_activePixels = active;
	return cov;
}

bool PixelBox::getDataValue(int x, int y, double *v)
{
	int pos = y * width + x;
	if (coverage && boxData && coverage[pos] && boxDataMap[pos]) {
		int offset = pos * channel * typeSize;
		for (int i = 0; i < channel; i++)
			v[i] = boxData[offset + i * typeSize];
		return true;
	}
	return false;
}

bool PixelBox::getDataValue(int x, int y, QVariant *v)
{
	double value[channel];
	if (getDataValue(x, y, value)) {
		for (int i = 0; i < channel; i++)
			v[i].setValue(value[i]);
		return true;
	}
	return false;
}

double PixelBox::getMin(int _channel)
{
	return getBoundary(_channel, maxVal, boxDataMin);
}

double PixelBox::getMax(int _channel)
{
	return getBoundary(_channel, minVal, boxDataMax, true);
}

double PixelBox::getAbsMin(int _channel)
{
	return getBoundary(_channel, maxVal, boxDataMinAbs);
}

double PixelBox::getAbsMax(int _channel)
{
	return getBoundary(_channel, 0.0, boxDataMaxAbs, true);
}

QImage PixelBox::getByteImage(FBMapping mapping)
{
	QImage image(width, height, QImage::Format_RGB32);
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			int pos = x * y;
			int offset = pos * typeSize;
			if (boxDataMap[pos]) {
				/* data available */
				QColor color;
				for (int i = 0; i < 3; ++i) {
					if (channel < i)
						break;
					color.setRed(mapFromValue(mapping, boxData[offset + typeSize * i], i));
				}
				image.setPixel(x, y, color.rgb());
			} else {
				/* no data, print checkerboard */
				if (((x / 8) % 2) == ((y / 8) % 2))
					image.setPixel(x, y, QColor(255, 255, 255).rgb());
				else
					image.setPixel(x, y, QColor(204, 204, 204).rgb());
			}
		}
	}
	return image;
}

void PixelBox::setByteImageChannel(Channels _channel, QImage *image, Mapping *mapping,
										RangeMapping *rangeMapping, float minmax[], bool useAlpha)
{
	if (!coverage || !boxData) {
		dbgPrint(DBGLVL_ERROR, "TypedPixelBox::setByteImageChannel: no coverage or data!\n");
		return;
	}

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			QColor c(image->pixel(x, y));
			unsigned long idx = y * width + x;
			int offset = idx * typeSize + _channel * typeSize;
			int val = 0;
			if (coverage[idx] && boxDataMap[idx])
				val = getMappedValueI(boxData[offset], mapping, rangeMapping, minmax);
			else if (useAlpha)
				c.setAlpha(0);
			else
				val = ((x / 8) % 2) == ((y / 8) % 2) ? 255 : 204;

			switch (_channel) {
			case pbcRed:
				c.setRed(val);
				break;
			case pbcGreen:
				c.setGreen(val);
				break;
			case pbcBlue:
				c.setBlue(val);
				break;
			}

			image->setPixel(x, y, c.rgb());
		}
	}
}


bool PixelBox::isAllDataAvailable()
{
	if (!boxDataMap)
		return false;
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int pos = x * y;
			if (coverage[pos] && !boxDataMap[pos]) {
				dbgPrint(DBGLVL_INFO,
						"NOT ALL DATA AVILABLE, NEED READBACK =========================\n");
				return false;
			}
		}
	}
	return true;
}

void PixelBox::invalidateData()
{
	if (!boxDataMap)
		return;
	size_t depth = typeSize * channel;
	memset(boxDataMap, 0, width * height * sizeof(bool));
	memset(boxDataMin, 0, depth);
	memset(boxDataMax, 0, depth);
	memset(boxDataMinAbs, 0, depth);
	memset(boxDataMaxAbs, 0, depth);
	emit dataChanged();
}


void PixelBox::clear()
{
	if (boxData)
		free(boxData);
	if (boxDataMap)
		free(boxDataMap);
	if(boxDataMin)
		free(boxDataMin);
	if (boxDataMax)
		free(boxDataMax);
	if (boxDataMinAbs)
		free(boxDataMinAbs);
	if (boxDataMaxAbs)
		free(boxDataMaxAbs);
	boxData = NULL;
	boxDataMap = NULL;
	boxDataMin = NULL;
	boxDataMax = NULL;
	boxDataMinAbs = NULL;
	boxDataMaxAbs = NULL;
	emit dataDeleted();
}

double PixelBox::getBoundary(int current_channel, int bval, void *data, bool max)
{
	if (current_channel < 0) {
		double result = bval;
		for (int c = 0; c < channel; c++) {
			double val = data[c * typeSize];
			if (max ? (val > result) : (val < result))
				result = val;
		}
		return (double) min;
	} else if (current_channel < channel) {
		return (double) data[current_channel * typeSize];
	} else {
		// TODO: Should this be silent?
		dbgPrint(DBGLVL_INTERNAL_WARNING, "%s is not valid channel in getBoundary.",
				 current_channel);
		return 0.0;
	}
}

int PixelBox::mapFromValue(FBMapping mapping, double f, int c)
{
	if (mapping == FBM_MIN_MAX) {
		int offset = c * typeSize;
		return (int) (255 * (f - boxDataMin[offset])
				/ (double) (boxDataMax[offset] - boxDataMin[offset]));
	}
	return CLAMP((int)(f * 255), 0, 255);
}
