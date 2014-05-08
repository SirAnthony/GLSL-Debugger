/******************************************************************************

Copyright (C) 2006-2009 Institute for Visualization and Interactive Systems
(VIS), Universität Stuttgart.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice, this
    list of conditions and the following disclaimer in the documentation and/or
    other materials provided with the distribution.

  * Neither the name of the name of VIS, Universität Stuttgart nor the names
    of its contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#ifndef MAPPINGS_H
#define MAPPINGS_H

enum MapType {
	MAP_TYPE_BLACK = 0,
	MAP_TYPE_WHITE,
	MAP_TYPE_VAR,
	MAP_TYPE_OFF
};

struct Mapping {
	int index;
	MapType type;
};

int getIntFromMapping(Mapping m);
Mapping getMappingFromInt(int i);

enum RangeMap {
	RANGE_MAP_DEFAULT = 0,
	RANGE_MAP_POSITIVE,
	RANGE_MAP_NEGATIVE,
	RANGE_MAP_ABSOLUTE,
	RANGE_MAP_COUNT
};

struct RangeMapping {
	int index;
	RangeMap range;
};

int getIntFromRangeMapping(RangeMapping m);
RangeMapping getRangeMappingFromInt(int i);
float getMappedValueF(float v, enum RangeMap range,	float min, float max);
int getMappedValueI(float v, enum RangeMap range, float min, float max);

#endif

