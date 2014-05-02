
#include <string.h>

#define CLAMP(x, min, max) ((x) < (min) ? (min) : (x) > (max) ? (max) : (x))

template<typename vType> double get_min();
template<typename vType> double get_max();

template<typename vType>
void PixelBox::calcMinMax(QRect area)
{
	int left, right, top, bottom;
	for (int c = 0; c < channel; c++) {
		int offset = c * typeSize;
		boxDataMin[offset] = (vType) maxVal;
		boxDataMax[offset] = (vType) minVal;
		boxDataMinAbs[offset] = (vType) maxVal;
		boxDataMaxAbs[offset] = (vType) 0;
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
			for (int c = 0; c < channel; c++) {				
				int offset = c * typeSize;
				int idx = pos * typeSize + offset;
				double fabsval = fabs((double) boxData[idx]);
				if (boxData[idx] < boxDataMin[offset])
					boxDataMin[offset] = boxData[idx];
				if (boxDataMax[offset] < boxData[idx])
					boxDataMax[offset] = boxData[idx];
				if (fabsval < boxDataMinAbs[offset])
					boxDataMinAbs[offset] = fabsval;
				if (boxDataMaxAbs[offset] < fabsval)
					boxDataMaxAbs[offset] = fabsval;
			}
		}
	}
}


template<typename vType>
void PixelBox::setData(int _width, int _height, int _channel,
							vType *_data, bool *_coverage)
{
	clear();
	width = _width;
	height = _height;
	channel = _channel;
	typeSize = sizeof(vType);
	minVal = get_min<vType>();
	maxVal = get_max<vType>();

	size_t depth = channel * typeSize;
	boxData = malloc(width * height * depth);
	boxDataMap = malloc(width * height * sizeof(bool));

	/* Initially use all given data */
	if (_data)
		memcpy(boxData, _data, width * height * depth);
	else
		memset(boxData, 0, width * height * depth);

	/* Initially use all given data */
	if (_coverage) {
		memcpy(boxDataMap, _coverage, width * height * sizeof(bool));
	} else {
		for (int i = 0; i < width * height; i++)
			boxDataMap[i] = true;
	}
	coverage = _coverage;

	if (_channel && _data) {
		boxDataMin = malloc(depth);
		boxDataMax = malloc(depth);
		boxDataMinAbs = malloc(depth);
		boxDataMaxAbs = malloc(depth);
		calcMinMax(this->minMaxArea);
	}

	emit dataChanged();
}
