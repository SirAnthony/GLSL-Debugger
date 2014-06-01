
#ifndef PIXEL_BOX_H
#define PIXEL_BOX_H

#include "ShaderLang.h"
#include "dataBox.h"
#include "mappings.h"
#include <QImage>
#include <QRect>

enum ImageChannels {
	icRed = 0,
	icGreen,
	icBlue
};

class PixelBox: public DataBox {
Q_OBJECT

public:
	enum FBMapping {
		FBM_CLAMP,
		FBM_MIN_MAX
	};

	PixelBox(QObject *parent = 0);
	PixelBox(PixelBox *src);
	virtual ~PixelBox();

	void copyFrom(DataBox *);

	int getWidth(void) { return width; }
	int getHeight(void) { return height; }
	int getChannels(void) { return channels; }

	template<typename vType>
	void setData(int width, int height, int channels, vType *data, bool *coverage = 0);
	int getSize() { return width * height; }
	int getDataSize() { return channels; }	

	const void* getDataPointer() { return boxData; }
	bool getDataValue(int x, int y, double *v);
	virtual bool getDataValue(int, QVariant *) { return false; }
	virtual bool getDataValue(int x, int y, QVariant *v);

	/* get min/max data values per channel, channel == -1 means all channels */
	virtual double getMin(int _channel = -1);
	virtual double getMax(int _channel = -1);
	virtual double getAbsMin(int _channel = -1);
	virtual double getAbsMax(int _channel = -1);

	QImage getByteImage(enum FBMapping mapping);
	void setByteImageChannel(enum ImageChannels, QImage *image, RangeMap range,
							 float minmax[2], bool useAlpha);

	bool isAllDataAvailable();
	void invalidateData();

signals:
	void minMaxAreaChanged();

public slots:
	void setMinMaxArea(const QRect& minMaxArea);

protected:
	void deleteData(bool signal = false);
	double getBoundary(int _channel, double val, void *data, bool max = false);
	int mapFromValue(FBMapping mapping, double f, int c);
	double getData(const void *data, int offset);

	template<typename vType>
	inline double getDataTyped(const void *data, int index);

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
	int channels;
	int channelsLen;
	QRect minMaxArea;
};

// include template definitions
#include "pixelBox.inc"

#endif /* PIXEL_BOX_H */

