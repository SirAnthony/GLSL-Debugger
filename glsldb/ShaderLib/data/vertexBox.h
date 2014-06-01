
#ifndef VERTEX_BOX_H
#define VERTEX_BOX_H

#include "dataBox.h"
#include <QtCore/QVariant>


class VertexBox: public DataBox {
Q_OBJECT

public:
	VertexBox(QObject *parent = 0);
	VertexBox(VertexBox* src, QObject *parent = 0);
	~VertexBox();

	void copyFrom(DataBox* src);

	void setData(float *data, int elementsPerVertex, int vertices,
			int primitives, bool *coverage = 0);

	void addVertexBox(VertexBox *f);
	float getDataValue(int index)
	{ return boxData ? boxData[index] : 0.0; }

	int getSize() { return verticesCount; }
	int getDataSize() { return verticesCount; }
	const void* getDataPointer() { return boxData; }
	bool getDataValue(int numVertex, float *v);
	virtual bool getDataValue(int numVertex, QVariant *v);

	int getNumElementsPerVertex(void) { return numElementsPerVertex; }
	int getNumVertices(void) { return verticesCount; }
	int getNumPrimitives(void) { return primitivesCount; }

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
	double getData(const void *data, int offset);
	void deleteData(bool signal = false);
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
