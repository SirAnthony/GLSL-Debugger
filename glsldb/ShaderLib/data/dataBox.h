#ifndef DATABOX_H
#define DATABOX_H

#include <QtCore/QObject>

class DataBox: public QObject {
Q_OBJECT

public:
	explicit DataBox(QObject *parent);
	/* get min/max data values per element, channel == -1 means all elements */
	virtual double getMin(int element = -1) = 0;
	virtual double getMax(int element = -1) = 0;
	virtual double getAbsMin(int element = -1) = 0;
	virtual double getAbsMax(int element = -1) = 0;

	void setNewCoverage(bool* _coverage)
	{
		coverage = _coverage;
	}

	bool* getCoveragePointer(void)
	{
		return coverage;
	}
	bool* getDataMapPointer(void)
	{
		return boxDataMap;
	}

signals:
	void dataChanged();
	void dataDeleted();

protected:
	bool *boxDataMap;
	bool *coverage;
};

#endif // DATABOX_H
