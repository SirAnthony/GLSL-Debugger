#ifndef DATABOX_H
#define DATABOX_H

#include <QObject>

class DataBox: public QObject {
Q_OBJECT

public:
	explicit DataBox(QObject *parent) : QObject(parent) { }
	explicit DataBox(bool *dataMap, bool *cov, QObject *parent)
		: QObject(parent), boxDataMap(dataMap), coverage(cov) {}
	~DataBox() {}

	/* get min/max data values per element, channel == -1 means all elements */
	virtual double getMin(int element = -1) = 0;
	virtual double getMax(int element = -1) = 0;
	virtual double getAbsMin(int element = -1) = 0;
	virtual double getAbsMax(int element = -1) = 0;

	void setNewCoverage(bool* _coverage)
	{ coverage = _coverage; }

	bool getCoverageValue(int index)
	{ return coverage ? coverage[index] : false; }

	virtual bool getDataValue(int, QVariant *) = 0;
	virtual bool getDataValue(int x, int, QVariant *v)
	{ return getDataValue(x, v); }

	bool getDataMapValue(int index)
	{ return boxDataMap ? boxDataMap[index] : false; }

	const bool* getCoveragePointer(void)
	{ return coverage; }
	const bool* getDataMapPointer(void)
	{ return boxDataMap; }

signals:
	void dataChanged();
	void dataDeleted();

protected:
	bool *boxDataMap;
	bool *coverage;
};

#endif // DATABOX_H



