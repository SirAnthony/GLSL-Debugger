
#ifndef PIXEL_BOX_H
#define PIXEL_BOX_H

#include "ShaderLang.h"
#include "dataBox.h"
#include "mappings.h"
#include <QtGui/QImage>


class PixelBox: public DataBox {
Q_OBJECT

public:
	PixelBox(QObject *parent = 0);
	virtual ~PixelBox();

	virtual bool* getCoverageFromData(int *activePixels = NULL) = 0;

	int getWidth(void)
	{
		return width;
	}
	int getHeight(void)
	{
		return height;
	}
	int getChannel(void)
	{
		return channel;
	}

	enum FBMapping {
		FBM_CLAMP,
		FBM_MIN_MAX
	};
	virtual QImage getByteImage(FBMapping i_eMapping) = 0;
	virtual void setByteImageRedChannel(QImage *image, Mapping *mapping,
			RangeMapping *rangeMapping, float minmax[2], bool useAlpha) = 0;
	virtual void setByteImageGreenChannel(QImage *image, Mapping *mapping,
			RangeMapping *rangeMapping, float minmax[2], bool useAlpha) = 0;
	virtual void setByteImageBlueChannel(QImage *image, Mapping *mapping,
			RangeMapping *rangeMapping, float minmax[2], bool useAlpha) = 0;

	virtual bool getDataValue(int x, int y, QVariant *v) = 0;

	bool isAllDataAvailable();
	virtual void invalidateData() = 0;

signals:
	void minMaxAreaChanged();

public slots:
	void setMinMaxArea(const QRect& minMaxArea);

protected:
	int width;
	int height;
	int channel;
	QRect minMaxArea;
};


template<typename vType> class TypedPixelBox: public PixelBox {
public:
	TypedPixelBox(int i_nWidth, int i_nHeight, int i_nChannel, vType *i_pData,
			bool *i_pCoverage = 0, QObject *i_qParent = 0);
	TypedPixelBox(TypedPixelBox *src);
	virtual ~TypedPixelBox();

	void setData(int i_nWidth, int i_nHeight, int i_nChannel, vType *i_pData,
			bool *i_pCoverage = 0);
	void addPixelBox(TypedPixelBox *f);

	virtual bool* getCoverageFromData(int *i_pActivePixels = NULL);
	vType* getDataPointer(void)
	{
		return boxData;
	}
	bool getDataValue(int x, int y, vType *v);
	virtual bool getDataValue(int x, int y, QVariant *v);

	/* get min/max data values per channel, channel == -1 means all channels */
	virtual double getMin(int channel = -1);
	virtual double getMax(int channel = -1);
	virtual double getAbsMin(int channel = -1);
	virtual double getAbsMax(int channel = -1);

	virtual QImage getByteImage(FBMapping i_eMapping);

	virtual void setByteImageRedChannel(QImage *image, Mapping *mapping,
			RangeMapping *rangeMapping, float minmax[2], bool useAlpha);
	virtual void setByteImageGreenChannel(QImage *image, Mapping *mapping,
			RangeMapping *rangeMapping, float minmax[2], bool useAlpha);
	virtual void setByteImageBlueChannel(QImage *image, Mapping *mapping,
			RangeMapping *rangeMapping, float minmax[2], bool useAlpha);

	virtual void invalidateData();

protected:

	static const vType minVal;
	static const vType maxVal;

	void calcMinMax(QRect area);
	int mapFromValue(FBMapping mapping, vType f, int c);

	vType *boxData;
	vType *boxDataMin;
	vType *boxDataMax;
	vType *boxDataMinAbs;
	vType *boxDataMaxAbs;
};

// include template definitions
#include "pixelBox.inc.h"

#endif /* PIXEL_BOX_H */

