
#ifndef PIXEL_BOX_H
#define PIXEL_BOX_H

#include "ShaderLang.h"
#include "dataBox.h"
#include "mappings.h"
#include <QImage>
#include <QRect>


class PixelBox: public DataBox {
Q_OBJECT

public:
	enum Channels {
		pbcRed = 0,
		pbcGreen,
		pbcBlue
	};
	enum FBMapping {
		FBM_CLAMP,
		FBM_MIN_MAX
	};

	PixelBox(QObject *parent = 0);
	PixelBox(PixelBox *src);
	virtual ~PixelBox();
	int getWidth(void) { return width; }
	int getHeight(void) { return height; }
	int getChannel(void) { return channel; }

	template<typename vType>
	void setData(int width, int height, int channel, vType *data, bool *coverage = 0);
	bool* getCoverageFromData(int *activePixels = NULL);
	void* getDataPointer(void) { return boxData; }
	bool getDataValue(int x, int y, double *v);
	bool getDataValue(int x, int y, QVariant *v);

	/* get min/max data values per channel, channel == -1 means all channels */
	virtual double getMin(int _channel = -1);
	virtual double getMax(int _channel = -1);
	virtual double getAbsMin(int _channel = -1);
	virtual double getAbsMax(int _channel = -1);

	QImage getByteImage(enum FBMapping mapping);
	void setByteImageChannel(enum Channels, QImage *image, RangeMap range,
							 float minmax[2], bool useAlpha);
	inline void setByteImageRedChannel(QImage *image, RangeMap range,
									   float minmax[2], bool useAlpha)
	{
		setByteImageChannel(pbcRed, image, range, minmax, useAlpha);
	}
	inline void setByteImageGreenChannel(QImage *image, RangeMap range,
										 float minmax[2], bool useAlpha)
	{
		setByteImageChannel(pbcGreen, image, range, minmax, useAlpha);
	}
	inline void setByteImageBlueChannel(QImage *image, RangeMap range,
										float minmax[2], bool useAlpha)
	{
		setByteImageChannel(pbcBlue, image, range, minmax, useAlpha);
	}

	bool isAllDataAvailable();
	void invalidateData();

signals:
	void minMaxAreaChanged();

public slots:
	void setMinMaxArea(const QRect& minMaxArea);

protected:
	void clear();
	double getBoundary(int _channel, int val, void *data, bool max = false);
	int mapFromValue(FBMapping mapping, double f, int c);
	double getData(void *data, int offset);

	template<typename vType>
	inline double getDataTyped(void *data, int index);

	template<typename vType>
	void setType();
	template<typename vType>
	void calcMinMax(QRect area);
	double minVal;
	double maxVal;

	enum DataType {
		dtError,
		dtUnsigned,
		dtInt,
		dtFloat,
		dtCount
	} dataType;
	void *boxData;
	void *boxDataMin;
	void *boxDataMax;
	void *boxDataMinAbs;
	void *boxDataMaxAbs;

	int width;
	int height;
	int channel;
	int channelLen;
	QRect minMaxArea;
};

// include template definitions
#include "pixelBox.inc"

#endif /* PIXEL_BOX_H */

