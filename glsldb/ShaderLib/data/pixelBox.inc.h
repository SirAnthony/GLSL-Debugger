/******************************************************************************

Copyright (C) 2006-2009 Institute for Visualization and Interactive Systems
(VIS), Universität Stuttgart.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice, this
	list of conditions and the following disclaimer in the documentation and/or
	other materials provided with the distribution.

  * Neither the name of the name of VIS, Universität Stuttgart nor the names
	of its contributors may be used to endorse or promote products derived from
	this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <math.h>
#include <string.h>

#include <QtGui/QColor>
#include <QtGui/QMessageBox>
#include <QtCore/QVariant>

#include "utils/dbgprint.h"

#define CLAMP(x, min, max) ((x) < (min) ? (min) : (x) > (max) ? (max) : (x))

template<typename vType>
TypedPixelBox<vType>::TypedPixelBox(int _width, int _height, int _channel,
		vType *_data, bool *_coverage, QObject *_parent) :
		PixelBox(_parent)
{
	width = _width;
	height = _height;
	channel = _channel;

	boxData = new vType[width * height * channel];
	boxDataMap = new bool[width * height];

	/* Initially use all given data */
	if (_data)
		memcpy(boxData, _data, width * height * channel * sizeof(vType));
	else
		memset(boxData, 0, width * height * channel * sizeof(vType));

	/* Initially use all given data */
	if (_coverage) {
		memcpy(boxDataMap, _coverage, width * height * sizeof(bool));
	} else {
		for (int i = 0; i < width * height; i++)
			boxDataMap[i] = true;
	}
	coverage = _coverage;

	if (_channel && _data) {
		boxDataMin = new vType[channel];
		boxDataMax = new vType[channel];
		boxDataMinAbs = new vType[channel];
		boxDataMaxAbs = new vType[channel];
		calcMinMax(this->minMaxArea);
	} else {
		boxDataMin = NULL;
		boxDataMax = NULL;
		boxDataMinAbs = NULL;
		boxDataMaxAbs = NULL;
	}
}

template<typename vType>
TypedPixelBox<vType>::TypedPixelBox(TypedPixelBox *src) :
		PixelBox(src->parent())
{
	width = src->width;
	height = src->height;
	channel = src->channel;
	coverage = src->coverage;

	boxData = new vType[width * height * channel];
	boxDataMap = new bool[width * height];

	memcpy(boxData, src->boxData,
			width * height * channel * sizeof(vType));
	memcpy(boxDataMap, src->boxDataMap, width * height * sizeof(bool));

	boxDataMin = new vType[channel];
	boxDataMax = new vType[channel];
	boxDataMinAbs = new vType[channel];
	boxDataMaxAbs = new vType[channel];

	memcpy(boxDataMin, src->boxDataMin, channel * sizeof(vType));
	memcpy(boxDataMax, src->boxDataMax, channel * sizeof(vType));
	memcpy(boxDataMinAbs, src->boxDataMinAbs, channel * sizeof(vType));
	memcpy(boxDataMaxAbs, src->boxDataMaxAbs, channel * sizeof(vType));
}

template<typename vType>
TypedPixelBox<vType>::~TypedPixelBox()
{
	delete[] boxData;
	delete[] boxDataMin;
	delete[] boxDataMax;
	delete[] boxDataMinAbs;
	delete[] boxDataMaxAbs;
}

template<typename vType>
void TypedPixelBox<vType>::calcMinMax(QRect area)
{
	int left, right, top, bottom;

	for (int c = 0; c < channel; c++) {
		boxDataMin[c] = maxVal;
		boxDataMax[c] = minVal;
		boxDataMinAbs[c] = maxVal;
		boxDataMaxAbs[c] = (vType) 0;
	}

	bool *pCoverage = coverage ? coverage : boxDataMap;
	if (area.isEmpty()) {
		left = top = 0;
		right = width;
		bottom = height;
	} else {
		left = (area.left() < 0) ? 0 : area.left();
		top = (area.top() < 0) ? 0 : area.top();
		// Note: area.right() != area.left() + area.width(), bottom same.
		right = (area.left() + area.width() > width) ?
				width : area.left() + area.width();
		bottom =
				(area.top() + area.height() > height) ?
						height : area.top() + area.height();
	}
	/*area.setCoords(0, 0, 2, 2);*/

	for (int y = top; y < bottom; y++) {
		for (int x = left; x < right; x++) {
			for (int c = 0; c < channel; c++) {
				int idx = y * width + x;
				if (pCoverage[idx]) {
					idx += c;
					if (boxData[idx] < boxDataMin[c])
						boxDataMin[c] = boxData[idx];
					if (boxDataMax[c] < boxData[idx])
						boxDataMax[c] = boxData[idx];
					if (fabs((double) boxData[idx]) < boxDataMinAbs[c])
						boxDataMinAbs[c] = fabs((double) boxData[idx]);
					if (boxDataMaxAbs[c] < fabs((double) boxData[idx]))
						boxDataMaxAbs[c] = fabs((double) boxData[idx]);
				}
			}
		}
	}
}

template<typename vType>
double TypedPixelBox<vType>::getMin(int channel)
{
	if (channel == -1) {
		vType min = maxVal;
		calcMinMax(this->minMaxArea); /* FIXME: is it necessary? We already calculate min/max when the data changes */
		for (int c = 0; c < channel; c++) {
			if (boxDataMin[c] < min)
				min = boxDataMin[c];
		}
		return (double) min;
	} else if (channel >= 0 && channel < channel) {
		return (double) boxDataMin[channel];
	} else {
		// TODO: Should this be silent?
		QString msg;
		msg.append(QString::number(channel));
		msg.append(" is not valid channel in TypedPixelBox::getMin.");
		msg.append("<BR>Please report this probem to "
				"<A HREF=\"mailto:glsldevil@vis.uni-stuttgart.de\">"
				"glsldevil@vis.uni-stuttgart.de</A>.");
		QMessageBox::critical(NULL, "Internal Error", msg, QMessageBox::Ok);
		return 0.0;
	}
}

template<typename vType>
double TypedPixelBox<vType>::getMax(int channel)
{
	if (channel == -1) {
		vType max = minVal;
		calcMinMax(this->minMaxArea);
		for (int c = 0; c < channel; c++) {
			if (boxDataMax[c] > max)
				max = boxDataMax[c];
		}
		return (double) max;
	} else if (channel >= 0 && channel < channel) {
		return (double) boxDataMax[channel];
	} else {
		// TODO: Should this be silent?
		QString msg;
		msg.append(QString::number(channel));
		msg.append(" is not valid channel in TypedPixelBox::getMax.");
		msg.append("<BR>Please report this problem to "
				"<A HREF=\"mailto:glsldevil@vis.uni-stuttgart.de\">"
				"glsldevil@vis.uni-stuttgart.de</A>.");
		QMessageBox::critical(NULL, "Internal Error", msg, QMessageBox::Ok);
		return 0.0;
	}
}

template<typename vType>
double TypedPixelBox<vType>::getAbsMin(int channel)
{
	if (channel == -1) {
		vType min = maxVal;
		calcMinMax(this->minMaxArea);
		for (int c = 0; c < channel; c++) {
			if (boxDataMinAbs[c] < min) {
				min = boxDataMinAbs[c];
			}
		}
		return (double) min;
	} else if (channel >= 0 && channel < channel) {
		return (double) boxDataMinAbs[channel];
	} else {
		// TODO: Should this be silent?
		QString msg;
		msg.append(QString::number(channel));
		msg.append(" is not valid channel in TypedPixelBox::getAbsMin.");
		msg.append("<BR>Please report this probem to "
				"<A HREF=\"mailto:glsldevil@vis.uni-stuttgart.de\">"
				"glsldevil@vis.uni-stuttgart.de</A>.");
		QMessageBox::critical(NULL, "Internal Error", msg, QMessageBox::Ok);
		return (double) 0;
	}
}

template<typename vType>
double TypedPixelBox<vType>::getAbsMax(int channel)
{
	if (channel == -1) {
		vType max = 0.0f;
		calcMinMax(this->minMaxArea);
		for (int c = 0; c < channel; c++) {
			if (boxDataMaxAbs[c] > max)
				max = boxDataMaxAbs[c];
		}
		return (double) max;
	} else if (channel >= 0 && channel < channel) {
		return (double) boxDataMaxAbs[channel];
	} else {
		// TODO: Should this be silent?
		QString msg;
		msg.append(QString::number(channel));
		msg.append(" is not valid channel in TypedPixelBox::getAbsMax.");
		msg.append("<BR>Please report this probem to "
				"<A HREF=\"mailto:glsldevil@vis.uni-stuttgart.de\">"
				"glsldevil@vis.uni-stuttgart.de</A>.");
		QMessageBox::critical(NULL, "Internal Error", msg, QMessageBox::Ok);
		return 0.0;
	}
}

template<typename vType>
void TypedPixelBox<vType>::setData(int _width, int _height, int _channel,
		vType *_data, bool *_coverage)
{
	delete[] boxData;
	delete[] boxDataMap;
	delete[] boxDataMin;
	delete[] boxDataMax;
	delete[] boxDataMinAbs;
	delete[] boxDataMaxAbs;

	width = _width;
	height = _height;
	channel = _channel;

	boxData = new vType[width * height * channel];
	boxDataMap = new bool[width * height];

	/* Initially use all given data */
	if (_data)
		memcpy(boxData, _data, width * height * channel * sizeof(vType));
	else
		memset(boxData, 0, width * height * channel * sizeof(vType));

	/* Initially use all given data */
	if (_coverage) {
		memcpy(boxDataMap, _coverage, width * height * sizeof(bool));
	} else {
		for (int i = 0; i < width * height; i++)
			boxDataMap[i] = true;
	}
	coverage = _coverage;

	if (_channel && _data) {
		boxDataMin = new vType[channel];
		boxDataMax = new vType[channel];
		boxDataMinAbs = new vType[channel];
		boxDataMaxAbs = new vType[channel];
		calcMinMax(this->minMaxArea);
	} else {
		boxDataMin = NULL;
		boxDataMax = NULL;
		boxDataMinAbs = NULL;
		boxDataMaxAbs = NULL;
	}
}

template<typename vType>
void TypedPixelBox<vType>::addPixelBox(TypedPixelBox<vType> *f)
{
	int i, j;
	vType *pDstData, *pSrcData;
	bool *pDstDataMap, *pSrcDataMap;

	if (width != f->getWidth() || height != f->getHeight()
			|| channel != f->getChannel()) {
		return;
	}

	pDstData = boxData;
	pDstDataMap = boxDataMap;

	pSrcData = f->getDataPointer();
	pSrcDataMap = f->getDataMapPointer();

	for (i = 0; i < width * height; i++) {
		if (*pSrcDataMap) {
			*pDstDataMap = true;
			for (j = 0; j < channel; j++) {
				*pDstData = *pSrcData;
				pDstData++;
				pSrcData++;
			}
		} else {
			for (j = 0; j < channel; j++) {
				pDstData++;
				pSrcData++;
			}
		}
		pDstDataMap++;
		pSrcDataMap++;
	}

	calcMinMax(this->minMaxArea);
	emit dataChanged();
}

template<typename vType>
bool* TypedPixelBox<vType>::getCoverageFromData(int *_activePixels)
{
	int i, j;
	int nActivePixels;
	vType *pData;
	bool *pCoverage;
	bool *coverage = new bool[width * height];

	pCoverage = coverage;
	pData = boxData;
	nActivePixels = 0;

	for (i = 0; i < width * height; i++) {
		if (*pData > 0.5f) {
			*pCoverage = true;
			nActivePixels++;
		} else {
			*pCoverage = false;
		}
		for (j = 0; j < channel; j++) {
			pData++;
		}
		pCoverage++;
	}

	if (_activePixels) {
		*_activePixels = nActivePixels;
	}

	return coverage;
}

template<typename vType>
int TypedPixelBox<vType>::mapFromValue(FBMapping i_eMapping, vType i_nF,
		int i_nC)
{
	switch (i_eMapping) {
	case FBM_MIN_MAX:
		return (int) (255 * (i_nF - boxDataMin[i_nC])
				/ (double) (boxDataMax[i_nC] - boxDataMin[i_nC]));
	case FBM_CLAMP:
	default:
		return CLAMP((int)(i_nF * 255), 0, 255);
	}
}

template<typename vType>
QImage TypedPixelBox<vType>::getByteImage(FBMapping i_eMapping)
{
	int x, y, c;
	vType *pData;
	bool *pDataMap;

	QImage image(width, height, QImage::Format_RGB32);

	pData = boxData;
	pDataMap = boxDataMap;
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			if (*pDataMap) {
				/* data available */
				QColor color;
				if (0 < channel) {
					color.setRed(mapFromValue(i_eMapping, *pData, 0));
					pData++;
				}
				if (1 < channel) {
					color.setGreen(mapFromValue(i_eMapping, *pData, 1));
					pData++;
				}
				if (2 < channel) {
					color.setBlue(mapFromValue(i_eMapping, *pData, 2));
					pData++;
				}
				for (c = 3; c < channel; c++) {
					pData++;
				}
				image.setPixel(x, y, color.rgb());
			} else {
				/* no data, print checkerboard */
				if (((x / 8) % 2) == ((y / 8) % 2)) {
					image.setPixel(x, y, QColor(255, 255, 255).rgb());
				} else {
					image.setPixel(x, y, QColor(204, 204, 204).rgb());
				}
				for (c = 0; c < channel; c++) {
					pData++;
				}
			}
			pDataMap++;
		}
	}
	return image;
}

template<typename vType>
void TypedPixelBox<vType>::setByteImageRedChannel(QImage *image,
		Mapping *mapping, RangeMapping *rangeMapping, float minmax[2],
		bool useAlpha)
{
	int x, y;

	if (channel != 1) {
		dbgPrint(DBGLVL_ERROR, "TypedPixelBox::setByteImageRedChannel(..)"
		" too many channels!");
		return;
	}

	if (!coverage || !boxData) {
		dbgPrint(DBGLVL_ERROR, "TypedPixelBox::setByteImageRedChannel(..)"
		" no coverage or data!\n");
		return;
	}

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			QColor c(image->pixel(x, y));
			unsigned long idx = y * width + x;
			if (coverage[idx] && boxDataMap[idx]) {
				c.setRed(
						getMappedValueI(boxData[idx], mapping, rangeMapping,
								minmax));
			} else if (useAlpha) {
				c.setRed(0);
				c.setAlpha(0);
			} else {
				c.setRed(((x / 8) % 2) == ((y / 8) % 2) ? 255 : 204);
			}
			image->setPixel(x, y, c.rgb());
		}
	}
}

template<typename vType>
void TypedPixelBox<vType>::setByteImageGreenChannel(QImage *image,
		Mapping *mapping, RangeMapping *rangeMapping, float minmax[2],
		bool useAlpha)
{
	int x, y;

	if (channel != 1) {
		dbgPrint(DBGLVL_ERROR, "TypedPixelBox::setByteImageGreenChannel(..)"
		" too many channels!");
		return;
	}

	if (!coverage || !boxData) {
		dbgPrint(DBGLVL_ERROR, "TypedPixelBox::setByteImageGreenChannel(..)"
		" no coverage or data!\n");
		return;
	}

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			QColor c(image->pixel(x, y));
			unsigned long idx = y * width + x;
			if (coverage[idx] && boxDataMap[idx]) {
				c.setGreen(
						getMappedValueI(boxData[idx], mapping, rangeMapping,
								minmax));
			} else if (useAlpha) {
				c.setGreen(0);
				c.setAlpha(0);
			} else {
				c.setGreen(((x / 8) % 2) == ((y / 8) % 2) ? 255 : 204);
			}
			image->setPixel(x, y, c.rgb());
		}
	}
}

template<typename vType>
void TypedPixelBox<vType>::setByteImageBlueChannel(QImage *image,
		Mapping *mapping, RangeMapping *rangeMapping, float minmax[2],
		bool useAlpha)
{
	int x, y;

	if (channel != 1) {
		dbgPrint(DBGLVL_ERROR, "TypedPixelBox::setByteImageBlueChannel(..)"
		" too many channels!");
		return;
	}

	if (!coverage || !boxData) {
		dbgPrint(DBGLVL_ERROR, "TypedPixelBox::setByteImageBlueChannel(..)"
		" no coverage or data!\n");
		return;
	}

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			QColor c(image->pixel(x, y));
			unsigned long idx = y * width + x;
			if (coverage[idx] && boxDataMap[idx]) {
				c.setBlue(
						getMappedValueI(boxData[idx], mapping, rangeMapping,
								minmax));
			} else if (useAlpha) {
				c.setBlue(0);
				c.setAlpha(0);
			} else {
				c.setBlue(((x / 8) % 2) == ((y / 8) % 2) ? 255 : 204);
			}
			image->setPixel(x, y, c.rgb());
		}
	}
}

template<typename vType>
void TypedPixelBox<vType>::invalidateData()
{
	int x, y, c;
	bool *pDataMap;

	if (!boxDataMap) {
		return;
	}

	pDataMap = boxDataMap;
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			*pDataMap++ = false;
		}
	}
	for (c = 0; c < channel; c++) {
		boxDataMin[c] = (vType) 0;
		boxDataMax[c] = (vType) 0;
		boxDataMinAbs[c] = (vType) 0;
		boxDataMaxAbs[c] = (vType) 0;
	}
	emit dataChanged();
}

template<typename vType>
bool TypedPixelBox<vType>::getDataValue(int x, int y, vType *v)
{
	int i;
	if (coverage && boxData && coverage[y * width + x]
			&& boxDataMap[y * width + x]) {
		for (i = 0; i < channel; i++) {
			v[i] = boxData[channel * (y * width + x) + i];
		}
		return true;
	} else {
		return false;
	}
}

template<typename vType>
bool TypedPixelBox<vType>::getDataValue(int x, int y, QVariant *v)
{
	vType *value = new vType[channel];
	if (getDataValue(x, y, value)) {
		for (int i = 0; i < channel; i++) {
			v[i] = value[i];
		}
		delete[] value;
		return true;
	} else {
		delete[] value;
		return false;
	}
}

