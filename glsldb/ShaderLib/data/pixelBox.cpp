
#include <float.h>
#include <limits.h>
#include <math.h>
#include "pixelBox.h"
#include "utils/dbgprint.h"
#include <QColor>
#include <QVariant>


template<> double get_min<float>() { return -FLT_MAX; }
template<> double get_max<float>() { return FLT_MAX; }
template<> double get_min<int>() { return INT_MIN; }
template<> double get_max<int>() { return INT_MAX; }
template<> double get_min<unsigned int>() { return 0; }
template<> double get_max<unsigned int>() { return UINT_MAX; }

template<> void PixelBox::setType<int>() { dataType = dtInt; }
template<> void PixelBox::setType<unsigned int>() { dataType = dtUnsigned; }
template<> void PixelBox::setType<float>() { dataType = dtFloat; }


PixelBox::PixelBox(QObject *parent) :
		DataBox(parent)
{
	width = 0;
	height = 0;
	channels = 0;
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
	channels = src->channels;
	coverage = src->coverage;
	minVal = src->minVal;
	maxVal = src->maxVal;
	dataType = src->dataType;
	channelsLen = src->channelsLen;

	boxData = malloc(width * height * channelsLen);
	boxDataMap = (bool*)malloc(width * height * sizeof(bool));
	memcpy(boxData, src->boxData, width * height * channelsLen);
	memcpy(boxDataMap, src->boxDataMap, width * height * sizeof(bool));

	boxDataMin = malloc(channelsLen);
	boxDataMax = malloc(channelsLen);
	boxDataMinAbs = malloc(channelsLen);
	boxDataMaxAbs = malloc(channelsLen);
	memcpy(boxDataMin, src->boxDataMin, channelsLen);
	memcpy(boxDataMax, src->boxDataMax, channelsLen);
	memcpy(boxDataMinAbs, src->boxDataMinAbs, channelsLen);
	memcpy(boxDataMaxAbs, src->boxDataMaxAbs, channelsLen);
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

bool PixelBox::getDataValue(int x, int y, double *v)
{
	int pos = y * width + x;
	if (coverage && boxData && coverage[pos] && boxDataMap[pos]) {
		int offset = pos * channels;
		for (int i = 0; i < channels; i++)
			v[i] = getData(boxData, offset + i);
		return true;
	}
	return false;
}

bool PixelBox::getDataValue(int x, int y, QVariant *v)
{
	double value[channels];
	if (getDataValue(x, y, value)) {
		for (int i = 0; i < channels; i++)
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

QImage PixelBox::getByteImage(enum FBMapping mapping)
{
	QImage image(width, height, QImage::Format_RGB32);
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int pos = x * y;
			if (boxDataMap[pos]) {
				/* data available */
				QColor color;
				for (int i = 0; i < 3; ++i) {
					if (channels < i)
						break;
					color.setRed(mapFromValue(mapping, getData(boxData, pos + i), i));
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

void PixelBox::setByteImageChannel(ImageChannels _channel, QImage *image, RangeMap range,
								   float minmax[], bool useAlpha)
{
	if (!coverage || !boxData) {
		dbgPrint(DBGLVL_ERROR, "TypedPixelBox::setByteImageChannel: no coverage or data!\n");
		return;
	}

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			QColor c(image->pixel(x, y));
			unsigned long idx = y * width + x;
			int offset = idx + _channel;
			int val = 0;
			if (coverage[idx] && boxDataMap[idx])
				val = getMappedValueI(getData(boxData, offset), range, minmax[0], minmax[1]);
			else if (useAlpha)
				c.setAlpha(0);
			else
				val = ((x / 8) % 2) == ((y / 8) % 2) ? 255 : 204;

			switch (_channel) {
			case icRed:
				c.setRed(val);
				break;
			case icGreen:
				c.setGreen(val);
				break;
			case icBlue:
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
	memset(boxDataMap, 0, width * height * sizeof(bool));
	memset(boxDataMin, 0, channelsLen);
	memset(boxDataMax, 0, channelsLen);
	memset(boxDataMinAbs, 0, channelsLen);
	memset(boxDataMaxAbs, 0, channelsLen);
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

double PixelBox::getBoundary(int current_channel, double bval, void *data, bool max)
{
	if (current_channel < 0) {
		double result = bval;
		for (int c = 0; c < channels; c++) {
			double val = getData(data, c);
			if (max ? (val > result) : (val < result))
				result = val;
		}
		return result;
	} else if (current_channel < channels) {
		return getData(data, current_channel);
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
		double min = getData(boxDataMin, c);
		double max = getData(boxDataMax, c);
		return (int) (255 * (f - min) / (double) (max - min));
	}
	return CLAMP((int)(f * 255), 0, 255);
}

double PixelBox::getData(const void *data, int offset)
{
	switch (dataType) {
	case dtInt:
		return getDataTyped<int>(data, offset);
	case dtUnsigned:
		return getDataTyped<unsigned int>(data, offset);
	case dtFloat:
		return getDataTyped<float>(data, offset);
	default:
		dbgPrint(DBGLVL_ERROR, "Wrong data type");
		break;
	}
	return 0.0;
}
