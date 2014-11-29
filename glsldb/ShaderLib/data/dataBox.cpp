
#include "dataBox.h"


int DataBox::getCoverageFromData(bool **src, const bool *old, bool *changed)
{
	if (!src)
		return 0;

	int size = getDataSize();
	const void *data = getDataPointer();
	bool *covermap = new bool[size];
	bool updated = !old;
	int active = 0;

	for (int i = 0; i < size; i++) {
		covermap[i] = getData(data, i * size) > 0.5f;
		if (covermap[i])
			active++;
		if (!updated && covermap[i] != old[i])
			updated = true;
	}

	if (*src)
		delete[] *src;
	*src = covermap;

	if (changed)
		*changed = updated;
	return active;
}
