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

#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#endif /* _WIN32 */

#include "glenumerants.h"
#if (defined(GLSLDB_LINUX) || defined(GLSLDB_OSX))
#  include "../GL/glx.h"
#  include "../GL/glxext.h"
#else
#  include "../GL/wglext.h"
#endif
#include "generated/glenumerants.h"


static void concatenate(char **dst, const char *src)
{
	if (!src || strlen(src) == 0) {
		return;
	}

	if (*dst) {
		*dst = realloc(*dst, strlen(*dst) + strlen(src) + 1);
		if (!dst) {
			fprintf(stderr, "concatenate'ing strings failed\n");
			exit(1);
		}
		strcat(*dst, src);
	} else {
#ifdef _WIN32
		/* strdup is not ANSI but Microsoft-specific, _strdup is ISO C++. */
		if (!(*dst = _strdup(src))) {
#else /* _WIN32 */
		if (!(*dst = strdup(src))) {
#endif /* _WIN32 */
			fprintf(stderr, "concatenate'ing strings failed\n");
			exit(1);
		}
	}
}

static const char* lookupEnumInMap(EnumerantsMap map[], int e)
{
	int i = 0;
	while (map[i].string != NULL) {
		if (map[i].value == e) {
			/* assumes enums are unique! */
			return map[i].string;
		}
		i++;
	}
	return "UNKNOWN ENUM!";
}

const char *lookupEnum(GLenum e)
{
	return lookupEnumInMap(glEnumerantsMap, e);
}

const char *lookupExtEnum(int e)
{
	return lookupEnumInMap(extEnumerantsMap, e);
}


static void concatEnumsInMap(char** result, EnumerantsMap map[], int e)
{
	int i = 0;
	while (map[i].string != NULL) {
		if (map[i].value == e) {
			/* assumes enums are unique! */
			const char *s = map[i].string;
			concatenate(result, s);
			concatenate(result, ",");
		}
		i++;
	}
}

char *lookupAllEnum(GLenum e)
{
	char *result = NULL;

	concatenate(&result, "{");
	concatEnumsInMap(&result, glEnumerantsMap, e);
	concatEnumsInMap(&result, extEnumerantsMap, e);

	if (strlen(result) == 1) {
		result = realloc(result, 14 * sizeof(char));
		strcpy(result, "UNKNOWN ENUM!");
	} else {
		result[strlen(result) - 1] = '}';
	}
	return result;
}

char *dissectBitfield(GLbitfield b)
{
	char *result = NULL;
	int i = 0;

	/* find combinations */
	while (glBitfieldMap[i].string != NULL) {
		if ((glBitfieldMap[i].value & b) == glBitfieldMap[i].value) {
			if (result != NULL) {
				concatenate(&result, "|");
			}
			concatenate(&result, glBitfieldMap[i].string);
		}
		i++;
	}
	return result;
}
