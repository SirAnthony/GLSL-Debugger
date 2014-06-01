
#ifndef LOOPDATA_H
#define LOOPDATA_H

#include <QtGui/QStandardItemModel>
#include <QtCore/QObject>
#include <QtGui/QImage>
#include "shdatamanager.h"


#define MAX_LOOP_ITERATIONS 255


class LoopData: public QObject {
Q_OBJECT

public:
	LoopData(ShaderMode mode, DataBox *condition, QObject *parent = 0);
	~LoopData();

	void addLoopIteration(ShaderMode mode, DataBox *condition, int iteration);

	QStandardItemModel* getModel(void)
	{ return &_model; }

	const bool* getInitialCoverage(void)
	{ return initialCoverage; }
	const bool* getActualCoverage(void);
	const float* getActualCondition(void);

	PixelBox* getActualPixelBox(void)
	{ return fragmentData; }
	VertexBox* getActualVertexBox(void)
	{ return vertexData; }

	int getTotal(void)
	{ return totalValues; }
	int getActive(void)
	{ return activeValues; }
	int getDone(void)
	{ return doneValues; }
	int getOut(void)
	{ return outValues; }

	int getWidth(void);
	int getHeight(void);
	int getIteration(void)
	{ return iteration; }

	QImage getImage(void);

	bool isFragmentLoop(void)
	{ return (fragmentData != NULL); }
	bool isVertexLoop(void)
	{ return (vertexData != NULL); }

private:
	DataBox *initializeBox(ShaderMode mode);
	void updateStatistic(void);

	int iteration;
	bool *initialCoverage;
	PixelBox *fragmentData;
	VertexBox *vertexData;

	int totalValues;
	int activeValues;
	int doneValues;
	int outValues;

	QStandardItemModel _model;
};

#endif /* LOOPDATA_H */

