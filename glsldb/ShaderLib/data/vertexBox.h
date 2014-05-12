
#ifndef VERTEX_BOX_H
#define VERTEX_BOX_H

#include "dataBox.h"
#include <QtCore/QVariant>


class VertexBox: public DataBox {
Q_OBJECT

public:
	VertexBox(QObject *i_qParent = 0);
	~VertexBox();

	void copyFrom(VertexBox* src);

	void setData(float *data, int elementsPerVertex, int vertices,
			int primitives, bool *coverage = 0);

	void addVertexBox(VertexBox *f);

	bool* getCoverageFromData(bool *oldCoverage, bool *coverageChanged);

	float getDataValue(int index)
	{
		return boxData ? boxData[index] : 0.0;
	}
	const float* getDataPointer(void)
	{
		return boxData;
	}

	bool getDataValue(int numVertex, float *v);
	bool getDataValue(int numVertex, QVariant *v);
	int getNumElementsPerVertex(void)
	{
		return numElementsPerVertex;
	}
	int getNumVertices(void)
	{
		return verticesCount;
	}
	int getNumPrimitives(void)
	{
		return primitivesCount;
	}

	void invalidateData();

	/* get min/max data values per element list, element == -1 means all
	 * elements of all vertices
	 */
	double getBoundary(int element, double bval, float *data, bool max = false);
	virtual double getMin(int element = -1);
	virtual double getMax(int element = -1);
	virtual double getAbsMin(int element = -1);
	virtual double getAbsMax(int element = -1);

private:
	void calcMinMax();

	float *boxData;
	int numElementsPerVertex;
	int verticesCount;
	int primitivesCount;
	float *boxDataMin;
	float *boxDataMax;
	float *boxDataMinAbs;
	float *boxDataMaxAbs;
};

#endif /* VERTEX_BOX_H */
