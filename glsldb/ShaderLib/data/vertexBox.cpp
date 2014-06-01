
#include <math.h>
#include <float.h>

#include "vertexBox.h"
#include "utils/dbgprint.h"

VertexBox::VertexBox(QObject *parent) :
		DataBox(parent)
{
	boxData = NULL;
	numElementsPerVertex = 0;
	verticesCount = 0;
	primitivesCount = 0;
	boxDataMap = NULL;
	coverage = NULL;
	boxDataMin = NULL;
	boxDataMax = NULL;
	boxDataMinAbs = NULL;
	boxDataMaxAbs = NULL;
}

VertexBox::VertexBox(VertexBox *src, QObject *parent)
	: DataBox(parent)
{
	numElementsPerVertex = 0;
	verticesCount = 0;
	primitivesCount = 0;
	coverage = NULL;
	boxData = NULL;
	boxDataMap = NULL;	
	boxDataMin = NULL;
	boxDataMax = NULL;
	boxDataMinAbs = NULL;
	boxDataMaxAbs = NULL;

	copyFrom(src);
}

VertexBox::~VertexBox()
{
	deleteData(true);
}

void VertexBox::copyFrom(DataBox *box)
{
	VertexBox *src = dynamic_cast<VertexBox *>(box);
	if (!src)
		return;

	deleteData();
	numElementsPerVertex = src->numElementsPerVertex;
	verticesCount = src->verticesCount;
	primitivesCount = src->primitivesCount;

	boxData = new float[verticesCount * numElementsPerVertex];
	boxDataMap = new bool[verticesCount];

	memcpy(boxData, src->boxData,
			verticesCount * numElementsPerVertex * sizeof(float));
	memcpy(boxDataMap, src->boxDataMap, verticesCount * sizeof(bool));

	coverage = src->coverage;

	boxDataMin = new float[numElementsPerVertex];
	boxDataMax = new float[numElementsPerVertex];
	boxDataMinAbs = new float[numElementsPerVertex];
	boxDataMaxAbs = new float[numElementsPerVertex];

	memcpy(boxDataMin, src->boxDataMin, numElementsPerVertex * sizeof(float));
	memcpy(boxDataMax, src->boxDataMax, numElementsPerVertex * sizeof(float));
	memcpy(boxDataMinAbs, src->boxDataMinAbs,
			numElementsPerVertex * sizeof(float));
	memcpy(boxDataMaxAbs, src->boxDataMaxAbs,
			numElementsPerVertex * sizeof(float));

	emit dataChanged();
}

void VertexBox::setData(float *data, int elementsPerVertex,
		int vertices, int primitives, bool *_coverage)
{
	if (verticesCount > 0)
		deleteData();

	if (vertices > 0 && data) {
		boxData = new float[vertices * elementsPerVertex];
		boxDataMap = new bool[vertices];
		memcpy(boxData, data,
				vertices * elementsPerVertex * sizeof(float));
		numElementsPerVertex = elementsPerVertex;
		verticesCount = vertices;
		primitivesCount = primitives;
		/* Initially use all given data */
		if (_coverage) {
			memcpy(boxDataMap, _coverage, verticesCount * sizeof(bool));
		} else {
			for (int i = 0; i < verticesCount; i++)
				boxDataMap[i] = true;
		}
		coverage = _coverage;

		boxDataMin = new float[numElementsPerVertex];
		boxDataMax = new float[numElementsPerVertex];
		boxDataMinAbs = new float[numElementsPerVertex];
		boxDataMaxAbs = new float[numElementsPerVertex];
		calcMinMax();
	}

	dbgPrint(DBGLVL_INFO,
			"VertexBox::setData: numPrimitives=%i numVertices=%i numElementsPerVertex=%i\n",
			 primitivesCount, verticesCount, numElementsPerVertex);

	emit dataChanged();
}

void VertexBox::addVertexBox(VertexBox *f)
{
	if (verticesCount != f->getNumVertices()
			|| primitivesCount != f->getNumPrimitives()
			|| numElementsPerVertex != f->getNumElementsPerVertex()) {
		return;
	}

	float *pDstData = boxData;
	bool *pDstDataMap = boxDataMap;
	const float *pSrcData = static_cast<const float*>(f->getDataPointer());
	const bool *pSrcDataMap = f->getDataMapPointer();

	for (int i = 0; i < verticesCount; i++) {
		for (int j = 0; j < numElementsPerVertex; j++) {
			if (*pSrcDataMap) {
				*pDstDataMap = true;
				*pDstData = *pSrcData;
			}
			pDstData++;
			pSrcData++;
		}
		pDstDataMap++;
		pSrcDataMap++;
	}
	calcMinMax();

	emit dataChanged();
}

bool VertexBox::getDataValue(int numVertex, float *v)
{
	bool *pCoverage = coverage ? coverage : boxDataMap;
	if (pCoverage && boxData && pCoverage[numVertex * numElementsPerVertex]) {
		for (int i = 0; i < numElementsPerVertex; i++)
			v[i] = boxData[numElementsPerVertex * numVertex + i];
		return true;
	}
	return false;
}

bool VertexBox::getDataValue(int numVertex, QVariant *v)
{
	bool *pCoverage = coverage ? coverage : boxDataMap;
	if (pCoverage && boxData && pCoverage[numVertex * numElementsPerVertex]) {
		for (int i = 0; i < numElementsPerVertex; i++)
			v[i] = boxData[numElementsPerVertex * numVertex + i];
		return true;
	}
	return false;
}

void VertexBox::invalidateData()
{
	if (!boxDataMap)
		return;

	dbgPrint(DBGLVL_DEBUG, "VertexBox::invalidateData()\n");
	bool *pDataMap = boxDataMap;

	for (int i = 0; i < verticesCount; i++)
		*pDataMap++ = false;

	for (int c = 0; c < numElementsPerVertex; c++) {
		boxDataMin[c] = 0.f;
		boxDataMax[c] = 0.f;
		boxDataMinAbs[c] = 0.f;
		boxDataMaxAbs[c] = 0.f;
	}
	emit dataChanged();
}

double VertexBox::getBoundary(int element, double bval, float *data, bool max)
{
	if (element < 0) {
		double result = bval;
		for (int c = 0; c < numElementsPerVertex; c++) {
			double val = data[c];
			if (max ? (val > result) : (val < result))
				result = val;
		}
		return result;
	} else if (element < numElementsPerVertex) {
		return data[element];
	}
	dbgPrint(DBGLVL_WARNING, "VertexBox::getBoundary: invalid element param %i\n", element);
	return 0.0;
}

double VertexBox::getMin(int element)
{
	return getBoundary(element, FLT_MAX, boxDataMin);
}

double VertexBox::getMax(int element)
{
	return getBoundary(element, -FLT_MAX, boxDataMax, true);
}

double VertexBox::getAbsMin(int element)
{
	return getBoundary(element, FLT_MAX, boxDataMinAbs);
}

double VertexBox::getAbsMax(int element)
{
	return getBoundary(element, 0.0, boxDataMaxAbs, true);
}

double VertexBox::getData(const void *data, int offset)
{
	const float* pData = static_cast<const float*>(data);
	return pData[offset];
}

void VertexBox::deleteData(bool signal)
{
	if (signal)
		emit dataDeleted();

	numElementsPerVertex = 0;
	verticesCount = 0;
	primitivesCount = 0;

	if (boxData)
		delete[] boxData, boxData = NULL;
	if (boxDataMap)
		delete[] boxDataMap, boxDataMap = NULL;
	if (boxDataMin)
		delete[] boxDataMin, boxDataMin = NULL;
	if (boxDataMax)
		delete[] boxDataMax, boxDataMax = NULL;
	if (boxDataMinAbs)
		delete[] boxDataMinAbs, boxDataMinAbs = NULL;
	if (boxDataMaxAbs)
		delete[] boxDataMaxAbs, boxDataMaxAbs = NULL;
}

void VertexBox::calcMinMax()
{
	for (int c = 0; c < numElementsPerVertex; c++) {
		boxDataMin[c] = FLT_MAX;
		boxDataMax[c] = -FLT_MAX;
		boxDataMinAbs[c] = FLT_MAX;
		boxDataMaxAbs[c] = 0.0f;
	}

	float *pData = boxData;
	bool *pCoverage = coverage ? coverage : boxDataMap;

	for (int v = 0; v < verticesCount; v++, pCoverage++) {
		for (int c = 0; c < numElementsPerVertex; c++, pData++) {
			if (!*pCoverage)
				continue;
			if (*pData < boxDataMin[c])
				boxDataMin[c] = *pData;
			if (boxDataMax[c] < *pData)
				boxDataMax[c] = *pData;
			if (fabs(*pData) < boxDataMinAbs[c])
				boxDataMinAbs[c] = fabs(*pData);
			if (boxDataMaxAbs[c] < fabs(*pData))
				boxDataMaxAbs[c] = fabs(*pData);
		}
	}
}
