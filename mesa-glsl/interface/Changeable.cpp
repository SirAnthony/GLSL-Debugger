/*
 * Changeable.cpp
 *
 *  Created on: 06.09.2013
 */

#include "Shader.h"
#include "AstScope.h"
#include "glsldb/utils/dbgprint.h"
#include <string.h>
#include <unordered_set>


void dumpShChangeable(const ShChangeable *cgb)
{
	if (cgb) {
		dbgPrint(DBGLVL_INFO, "%i", cgb->id);
		for (int j = 0; j < cgb->numIndices; j++) {
			const ShChangeableIndex *idx = cgb->indices[j];
			if (idx) {
				switch (idx->type) {
				case SH_CGB_ARRAY:
					dbgPrint(DBGLVL_INFO, "[%i]", idx->index);
					break;
				case SH_CGB_STRUCT:
					dbgPrint(DBGLVL_INFO, ".%i", idx->index);
					break;
				case SH_CGB_SWIZZLE:
					dbgPrint(DBGLVL_INFO, ",%i", idx->index);
					break;
				default:
					break;
				}
			}
		}
		dbgPrint(DBGLVL_INFO, " ");
	}
}

void dumpShChangeableList(ShChangeableList *cl)
{
	int i;

	if (!cl)
		return;
	dbgPrint(DBGLVL_INFO, "===> ");
	if (cl->numChangeables == 0) {
		dbgPrint(DBGLVL_INFO, "empty\n");
		return;
	}

	for (i = 0; i < cl->numChangeables; i++)
		dumpShChangeable(cl->changeables[i]);
	dbgPrint(DBGLVL_INFO, "\n");
}

static bool isEqualShChangeable(const ShChangeable *a, const ShChangeable *b)
{
	int i;

	if (a->id != b->id)
		return false;
	if (a->numIndices != b->numIndices)
		return false;

	for (i = 0; i < a->numIndices; i++) {
		if (a->indices[i]->type != b->indices[i]->type)
			return false;
		if (a->indices[i]->index != b->indices[i]->index)
			return false;
	}

	return true;
}

////////////////////////////////////////////////////////////////////////
// Old functions
//

void addShChangeable(ShChangeableList *cl, ShChangeable *c)
{
	addShChangeableCtx(cl, c);
}


////////////////////////////////////////////////////////////////////////
// Mem-safe functions
//

ShChangeable* createShChangeableCtx(int id, void* mem_ctx)
{
	ShChangeable *cgb = (ShChangeable*)rzalloc(mem_ctx, ShChangeable);
	cgb->id = id;
	return cgb;
}

ShChangeableIndex* createShChangeableIndexCtx(ShChangeableType type, long index, void* mem_ctx)
{
	ShChangeableIndex *idx = (ShChangeableIndex*)rzalloc(mem_ctx, ShChangeableIndex);
	idx->type = type;
	idx->index = index;
	return idx;
}

void addShChangeableCtx(ShChangeableList *cl, const ShChangeable *c)
{
	if (!cl || !c)
		return;

	cl->numChangeables++;
	cl->changeables = (const ShChangeable**) reralloc_array_size(cl, cl->changeables,
			sizeof(ShChangeable*), cl->numChangeables);
	cl->changeables[cl->numChangeables - 1] = c;
}

ShChangeable * copyShChangeableCtx(const ShChangeable *c, void* mem_ctx)
{
	int i;
	ShChangeable *copy;

	if (!c)
		return NULL;

	copy = createShChangeableCtx(c->id, mem_ctx);

	// add all indices
	for (i = 0; i < c->numIndices; i++) {
		copy->numIndices++;
		copy->indices = (const ShChangeableIndex**)reralloc_array_size(copy, copy->indices,
				sizeof(ShChangeableIndex*), copy->numIndices);

		copy->indices[copy->numIndices - 1] = createShChangeableIndexCtx(
						c->indices[i]->type, c->indices[i]->index, copy);
	}

	return copy;
}

void copyShChangeableToListCtx(ShChangeableList *cl, const ShChangeable *c)
{
	if (!cl || !c)
		return;

	ShChangeable *copy = copyShChangeableCtx(c, cl);
	addShChangeableCtx(cl, copy);
}

void copyShChangeableListCtx(ShChangeableList *clout, exec_list *clin)
{
	if (!clout || !clin)
		return;

	foreach_list(node, clin) {
		changeable_item* ch_item = (changeable_item*) node;
		bool alreadyInList = false;
		for (int j = 0; j < clout->numChangeables; j++) {
			if (isEqualShChangeable(clout->changeables[j], ch_item->changeable)) {
				alreadyInList = true;
				break;
			}
		}
		if (!alreadyInList)
			copyShChangeableToListCtx(clout, ch_item->changeable);
	}
}

void copyAstChangeableList(exec_list *clout, exec_list *clin, exec_list* only, void* mem_ctx)
{
	if (!clout || !clin || clin->is_empty())
		return;

	std::unordered_set<int> permits;
	if (only)
		foreach_list(node, only)
			permits.emplace(((scope_item*)node)->id);


	// TODO: this algorithm is bad.
	foreach_list(node, clin){
		changeable_item* ch_item = (changeable_item*)node;
		if (only && permits.find(ch_item->id) == permits.end())
			continue;
		bool alreadyInList = false;
		foreach_list(onode, clout) {
			changeable_item* cho_item = (changeable_item*)onode;
			if (!isEqualShChangeable(ch_item->changeable, cho_item->changeable))
				continue;
			alreadyInList = true;
			break;
		}
		if (!alreadyInList)
			clout->push_tail(ch_item->clone(mem_ctx));
	}
}

void addShIndexToChangeableCtx(ShChangeable *c, const ShChangeableIndex *idx)
{
	if (!c)
		return;

	c->numIndices++;
	c->indices = (const ShChangeableIndex**) reralloc_array_size(c, c->indices,
			sizeof(ShChangeableIndex*), c->numIndices);
	c->indices[c->numIndices - 1] = idx;
}

void freeShChangeable(ShChangeable **c)
{
	if (c && *c) {
		ralloc_free(*c);
		*c = NULL;
	}
}

