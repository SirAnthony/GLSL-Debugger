
#include <string.h>

#define CLAMP(x, min, max) ((x) < (min) ? (min) : (x) > (max) ? (max) : (x))

template<typename vType> double get_min();
template<typename vType> double get_max();

template<typename vType>
inline double PixelBox::getDataTyped(const void *data, int index)
{
	const vType *pData = static_cast<const vType *>(data);
	return pData[index];
}

template<typename vType>
void PixelBox::calcMinMax(QRect area)
{	
	vType *pBoxDataMin = boxDataMin;
	vType *pBoxDataMax = boxDataMax;
	vType *pBoxDataMinAbs = boxDataMinAbs;
	vType *pBoxDataMaxAbs = boxDataMaxAbs;
	vType *pBoxData = boxData;

	int left, right, top, bottom;
	for (int c = 0; c < channels; c++) {
		pBoxDataMin[c] = maxVal;
		pBoxDataMax[c] = minVal;
		pBoxDataMinAbs[c] = maxVal;
		pBoxDataMaxAbs[c] = 0;
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
		bottom = (area.top() + area.height() > height) ?
				height : area.top() + area.height();
	}

	for (int y = top; y < bottom; y++) {
		for (int x = left; x < right; x++) {
			int pos = y * width + x;
			if (!pCoverage[pos])
				continue;
			for (int c = 0; c < channels; c++) {
				int idx = pos + c;
				double fabsval = fabs((double) pBoxData[idx]);
				if (pBoxData[idx] < pBoxDataMin[c])
					pBoxDataMin[c] = pBoxData[idx];
				if (pBoxDataMax[c] < pBoxData[idx])
					pBoxDataMax[c] = pBoxData[idx];
				if (fabsval < pBoxDataMinAbs[c])
					pBoxDataMinAbs[c] = fabsval;
				if (pBoxDataMaxAbs[c] < fabsval)
					pBoxDataMaxAbs[c] = fabsval;
			}
		}
	}
}


template<typename vType>
void PixelBox::setData(int _width, int _height, int _channels,
							vType *_data, bool *_coverage)
{
	clear();
	width = _width;
	height = _height;
	channels = _channels;
	minVal = get_min<vType>();
	maxVal = get_max<vType>();
	setType<vType>();

	channelsLen = channels * sizeof(vType);
	boxData = malloc(width * height * channelsLen);
	boxDataMap = malloc(width * height * sizeof(bool));

	/* Initially use all given data */
	if (_data)
		memcpy(boxData, _data, width * height * channelsLen);
	else
		memset(boxData, 0, width * height * channelsLen);

	/* Initially use all given data */
	if (_coverage) {
		memcpy(boxDataMap, _coverage, width * height * sizeof(bool));
	} else {
		for (int i = 0; i < width * height; i++)
			boxDataMap[i] = true;
	}
	coverage = _coverage;

	if (_channels && _data) {
		boxDataMin = malloc(channelsLen);
		boxDataMax = malloc(channelsLen);
		boxDataMinAbs = malloc(channelsLen);
		boxDataMaxAbs = malloc(channelsLen);
		calcMinMax<vType>(this->minMaxArea);
	}

	emit dataChanged();
}
