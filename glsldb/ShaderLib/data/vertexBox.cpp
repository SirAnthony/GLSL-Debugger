
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

VertexBox::VertexBox(float *i_pData, int i_numElementsPerVertex,
		int i_numVertices, int i_numPrimitives, bool *i_pCoverage,
		QObject *parent) : DataBox(parent)
{
	setData(i_pData, i_numElementsPerVertex, i_numVertices, i_numPrimitives,
			i_pCoverage);
}

VertexBox::~VertexBox()
{
	emit dataDeleted();
	delete[] boxData;
	delete[] boxDataMap;
	delete[] boxDataMin;
	delete[] boxDataMax;
	delete[] boxDataMinAbs;
	delete[] boxDataMaxAbs;
}

void VertexBox::copyFrom(VertexBox *src)
{
	delete[] boxData;
	delete[] boxDataMap;
	delete[] boxDataMin;
	delete[] boxDataMax;
	delete[] boxDataMinAbs;
	delete[] boxDataMaxAbs;

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

}

void VertexBox::setData(float *data, int elementsPerVertex,
		int vertices, int primitives, bool *_coverage)
{
	if (verticesCount > 0) {
		delete[] boxData;
		delete[] boxDataMap;
		delete[] boxDataMin;
		delete[] boxDataMax;
		delete[] boxDataMinAbs;
		delete[] boxDataMaxAbs;
		boxData = NULL;
		boxDataMap = NULL;
	}
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
	} else {
		boxData = NULL;
		boxDataMap = NULL;
		coverage = NULL;
		numElementsPerVertex = 0;
		verticesCount = 0;
		primitivesCount = 0;
		boxDataMin = NULL;
		boxDataMax = NULL;
		boxDataMinAbs = NULL;
		boxDataMaxAbs = NULL;
	}

	dbgPrint(DBGLVL_INFO,
			"VertexBox::setData: numPrimitives=%i numVertices=%i numElementsPerVertex=%i\n",
			 primitivesCount, verticesCount, numElementsPerVertex);

	emit dataChanged();
}

bool* VertexBox::getCoverageFromData(bool *oldCoverage, bool *coverageChanged)
{
	bool *coverage = new bool[verticesCount];
	bool *pCoverage = coverage;
	bool *pOldCoverage = oldCoverage;
	float *pData = boxData;
	*coverageChanged = !oldCoverage;

	for (int i = 0; i < verticesCount; i++) {
		*pCoverage = bool(*pData > 0.5f);
		if (oldCoverage && *pCoverage != *pOldCoverage)
			*coverageChanged = true;
		pOldCoverage++;
		pCoverage++;
		pData += numElementsPerVertex;
	}
	return coverage;
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
	float *pSrcData = f->getDataPointer();
	bool *pSrcDataMap = f->getDataMapPointer();

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

double VertexBox::getMin(int element)
{
	if (element == -1) {
		float min = FLT_MAX;
		for (int c = 0; c < numElementsPerVertex; c++) {
			if (boxDataMin[c] < min)
				min = boxDataMin[c];
		}
		return min;
	} else if (element >= 0 && element < numElementsPerVertex) {
		return boxDataMin[element];
	}
	dbgPrint(DBGLVL_WARNING, "VertexBox::getMin: invalid element param %i\n", element);
	return 0.0f;
}

double VertexBox::getMax(int element)
{
	if (element == -1) {
		float max = -FLT_MAX;
		for (int c = 0; c < numElementsPerVertex; c++) {
			if (boxDataMax[c] > max)
				max = boxDataMax[c];
		}
		return max;
	} else if (element >= 0 && element < numElementsPerVertex) {
		return boxDataMax[element];
	}
	dbgPrint(DBGLVL_WARNING, "VertexBox::getMax: invalid element param %i\n", element);
	return 0.0f;
}

double VertexBox::getAbsMin(int element)
{
	if (element == -1) {
		float min = FLT_MAX;
		for (int c = 0; c < numElementsPerVertex; c++) {
			if (boxDataMinAbs[c] < min) {
				min = boxDataMinAbs[c];
			}
		}
		return min;
	} else if (element >= 0 && element < numElementsPerVertex) {
		return boxDataMinAbs[element];
	}
	dbgPrint(DBGLVL_WARNING, "VertexBox::getAbsMin: invalid element param %i\n", element);
	return 0.0f;
}

double VertexBox::getAbsMax(int element)
{
	if (element == -1) {
		float max = 0.0f;
		for (int c = 0; c < numElementsPerVertex; c++) {
			if (boxDataMaxAbs[c] > max) {
				max = boxDataMaxAbs[c];
			}
		}
		return max;
	} else if (element >= 0 && element < numElementsPerVertex) {
		return boxDataMaxAbs[element];
	}
	dbgPrint(DBGLVL_WARNING, "VertexBox::getAbsMax: invalid element param %i\n", element);
	return 0.0f;
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
