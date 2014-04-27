
#include "pixelBox.qt.h"
#include "dbgprint.h"

PixelBox::PixelBox(QObject *parent) :
		DataBox(parent)
{
	width = 0;
	height = 0;
	channel = 0;
	dataMap = NULL;
	coverage = NULL;
}

PixelBox::~PixelBox()
{
	emit dataDeleted();

	delete[] dataMap;
}

bool PixelBox::isAllDataAvailable()
{
	int x, y;
	bool *pDataMap;
	bool *pCoverage;

	if (!dataMap) {
		return false;
	}

	pDataMap = dataMap;
	pCoverage = coverage;

	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			if (*pCoverage && !*pDataMap) {
				dbgPrint(DBGLVL_INFO,
						"NOT ALL DATA AVILABLE, NEED READBACK =========================\n");
				return false;
			}
			pDataMap++;
			pCoverage++;
		}
	}
	return true;
}

void PixelBox::setMinMaxArea(const QRect& minMaxArea)
{
	this->minMaxArea = minMaxArea;
	emit minMaxAreaChanged();
}

template<> const float TypedPixelBox<float>::minVal = -FLT_MAX;
template<> const float TypedPixelBox<float>::maxVal = FLT_MAX;

template<> const int TypedPixelBox<int>::minVal = INT_MIN;
template<> const int TypedPixelBox<int>::maxVal = INT_MAX;

template<> const unsigned int TypedPixelBox<unsigned int>::minVal = 0;
template<> const unsigned int TypedPixelBox<unsigned int>::maxVal = UINT_MAX;

