/*
 * Changeable.cpp
 *
 *  Created on: 06.09.2013
 */

#include "ShaderLang.h"
#include "glsldb/utils/dbgprint.h"
#include <string.h>
#include <map>
#include "ir.h"
#include "ast.h"

typedef std::map<ast_declaration*, ShVariable*> VariableMap;

namespace {
std::map<int, ShVariable*> VariablesById;
VariableMap VariablesBySource;
unsigned int VariablesCount = 0;
}

static const char* getShTypeString(ShVariable *v)
{
	switch (v->type) {
	case SH_FLOAT:
		return "float";
	case SH_INT:
		return "int";
	case SH_UINT:
		return "unsigned int";
	case SH_BOOL:
		return "bool";
	case SH_STRUCT:
		return v->structName;
	case SH_ARRAY:
		return "array";
	case SH_SAMPLER_1D:
		return "sampler1D";
	case SH_ISAMPLER_1D:
		return "isampler1D";
	case SH_USAMPLER_1D:
		return "usampler1D";
	case SH_SAMPLER_2D:
		return "sampler2D";
	case SH_ISAMPLER_2D:
		return "isampler2D";
	case SH_USAMPLER_2D:
		return "usampler2D";
	case SH_SAMPLER_3D:
		return "sampler3D";
	case SH_ISAMPLER_3D:
		return "isampler3D";
	case SH_USAMPLER_3D:
		return "usampler3D";
	case SH_SAMPLER_CUBE:
		return "samplerCube";
	case SH_ISAMPLER_CUBE:
		return "isamplerCube";
	case SH_USAMPLER_CUBE:
		return "usamplerCube";
	case SH_SAMPLER_1D_SHADOW:
		return "sampler1DShadow";
	case SH_SAMPLER_2D_SHADOW:
		return "sampler2DShadow";
	case SH_SAMPLER_2D_RECT:
		return "sampler2DRect";
	case SH_ISAMPLER_2D_RECT:
		return "isampler2DRect";
	case SH_USAMPLER_2D_RECT:
		return "usampler2DRect";
	case SH_SAMPLER_2D_RECT_SHADOW:
		return "samplerRectShadow";
	case SH_SAMPLER_1D_ARRAY:
		return "sampler1DArray";
	case SH_ISAMPLER_1D_ARRAY:
		return "isampler1DArray";
	case SH_USAMPLER_1D_ARRAY:
		return "usampler1DArray";
	case SH_SAMPLER_2D_ARRAY:
		return "sampler2DArray";
	case SH_ISAMPLER_2D_ARRAY:
		return "isampler2DArray";
	case SH_USAMPLER_2D_ARRAY:
		return "usampler2DArray";
	case SH_SAMPLER_BUFFER:
		return "samplerBuffer";
	case SH_ISAMPLER_BUFFER:
		return "isamplerBuffer";
	case SH_USAMPLER_BUFFER:
		return "usamplerBuffer";
	case SH_SAMPLER_1D_ARRAY_SHADOW:
		return "sampler1DArrayShadow";
	case SH_SAMPLER_2D_ARRAY_SHADOW:
		return "sampler2DArrayShadow";
	case SH_SAMPLER_CUBE_SHADOW:
		return "samplerCubeShadow";
	default:
		return "unknown";
	}
}

variableQualifier qualifierToNative(ast_type_qualifier* qualifier, bool is_parameter)
{
	int qual = SH_UNSET;
	qual |= qualifier->flags.q.uniform * SH_UNIFORM;
	qual |= qualifier->flags.q.in * SH_IN;
	qual |= qualifier->flags.q.out * SH_OUT;
	qual |= qualifier->flags.q.varying * SH_VARYING;
	qual |= qualifier->flags.q.attribute * SH_ATTRIBUTE;
	qual |= qualifier->flags.q.constant * SH_CONST;
	qual |= is_parameter * SH_PARAM;
	if (!qual)
		qual = SH_GLOBAL;

	return (variableQualifier)qual;
	/* TODO: not sure about origin
		SH_BUILTIN_READ
		SH_BUILTIN_WRITE
	*/
}

bool ShIsSampler(variableType v)
{
	if (v < SH_SAMPLER_GUARD_BEGIN || SH_SAMPLER_GUARD_END < v) {
		return false;
	} else {
		return true;
	}
}

static const char* getShQualifierString(variableQualifier q)
{
	switch (q) {
	case SH_UNSET:
		return "";
	case SH_TEMPORARY:
		return "temporary";
	case SH_GLOBAL:
		return "global";
	case SH_CONST:
		return "const";
	case SH_ATTRIBUTE:
		return "attribute";
	case SH_VARYING_IN:
		return "varying_in";
	case SH_VARYING_OUT:
		return "varying_out";
	case SH_UNIFORM:
		return "uniform";
	case SH_PARAM_IN:
		return "parameter_in";
	case SH_PARAM_OUT:
		return "parameter_out";
	case SH_PARAM_INOUT:
		return "parameter_inout";
	case SH_PARAM_CONST:
		return "parameter_const";
	case SH_BUILTIN_READ:
		return "builtin_read";
	case SH_BUILTIN_WRITE:
		return "builtin_write";
	default:
		return "unknown";
	}
}

//
//
// CHANGEABLES helper functions
//
//

void dumpShChangeable(ShChangeable *cgb)
{
	int j;

	if (cgb) {
		dbgPrint(DBGLVL_INFO, "%i", cgb->id);
		for (j = 0; j < cgb->numIndices; j++) {
			ShChangeableIndex *idx = cgb->indices[j];
			if (idx) {
				switch (idx->type) {
				case SH_CGB_ARRAY_DIRECT:
					dbgPrint(DBGLVL_INFO, "[%i]", idx->index);
					break;
				case SH_CGB_ARRAY_INDIRECT:
					dbgPrint(DBGLVL_INFO, "[(%i)]", idx->index);
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

	for (i = 0; i < cl->numChangeables; i++) {
		ShChangeable *cgb = cl->changeables[i];
		dumpShChangeable(cgb);
	}
	dbgPrint(DBGLVL_INFO, "\n");
}

ShChangeable* createShChangeable(int id)
{
	ShChangeable *cgb;
	if (!(cgb = (ShChangeable*) malloc(sizeof(ShChangeable)))) {
		dbgPrint(DBGLVL_ERROR, "not enough memory for cgb\n");
	}
	cgb->id = id;
	cgb->numIndices = 0;
	cgb->indices = NULL;
	return cgb;
}

ShChangeableIndex* createShChangeableIndex(ShChangeableType type, int index)
{
	ShChangeableIndex *idx;
	if (!(idx = (ShChangeableIndex*) malloc(sizeof(ShChangeableIndex)))) {
		dbgPrint(DBGLVL_ERROR, "not enough memory for idx\n");
		exit(1);
	}

	idx->type = type;
	idx->index = index;

	return idx;
}

void addShChangeable(ShChangeableList *cl, ShChangeable *c)
{
	if (!cl || !c)
		return;

	cl->numChangeables++;
	cl->changeables = (ShChangeable**) realloc(cl->changeables,
			cl->numChangeables * sizeof(ShChangeable*));
	cl->changeables[cl->numChangeables - 1] = c;
}

void copyShChangeable(ShChangeableList *cl, ShChangeable *c)
{
	int i;
	ShChangeable *copy;

	if (!cl || !c)
		return;

	cl->numChangeables++;
	cl->changeables = (ShChangeable**) realloc(cl->changeables,
			cl->numChangeables * sizeof(ShChangeable*));

	copy = createShChangeable(c->id);

	// add all indices
	for (i = 0; i < c->numIndices; i++) {
		copy->numIndices++;
		copy->indices = (ShChangeableIndex**) realloc(copy->indices,
				copy->numIndices * sizeof(ShChangeableIndex*));

		copy->indices[copy->numIndices - 1] = (ShChangeableIndex*) malloc(
				sizeof(ShChangeableIndex));

		copy->indices[copy->numIndices - 1]->type = c->indices[i]->type;
		copy->indices[copy->numIndices - 1]->index = c->indices[i]->index;
	}

	cl->changeables[cl->numChangeables - 1] = copy;
}

static bool isEqualShChangeable(ShChangeable *a, ShChangeable *b)
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

void copyShChangeableList(ShChangeableList *clout, ShChangeableList *clin)
{
	int i, j;

	if (!clout || !clin)
		return;

	for (i = 0; i < clin->numChangeables; i++) {
		// copy only if not already in list
		bool alreadyInList = false;
		for (j = 0; j < clout->numChangeables; j++) {
			if (isEqualShChangeable(clout->changeables[j],
					clin->changeables[i])) {
				alreadyInList = true;
				break;
			}
		}
		if (!alreadyInList) {
			copyShChangeable(clout, clin->changeables[i]);
		}
	}
}

void addShIndexToChangeable(ShChangeable *c, ShChangeableIndex *idx)
{
	if (!c)
		return;

	c->numIndices++;
	c->indices = (ShChangeableIndex**) realloc(c->indices,
			c->numIndices * sizeof(ShChangeableIndex*));
	c->indices[c->numIndices - 1] = idx;
}

void addShIndexToChangeableList(ShChangeableList *cl, int s,
		ShChangeableIndex *idx)
{
	if (!cl)
		return;

	if (s < cl->numChangeables) {
		addShIndexToChangeable(cl->changeables[s], idx);
	}
}

void freeShChangeable(ShChangeable **c)
{
	if (c && *c) {
		int i;
		for (i = 0; i < (*c)->numIndices; i++) {
			free((*c)->indices[i]);
		}
		free((*c)->indices);
		free(*c);
		*c = NULL;
	}
}

void addShVariable(ShVariableList *vl, ShVariable *v, int builtin)
{
	int i;
	ShVariable **vp = vl->variables;

	v->builtin = builtin;

	for (i = 0; i < vl->numVariables; i++) {
		if (strcmp(vp[i]->name, v->name) == 0) {
			vp[i] = v;
			return;
		}
	}

	vl->numVariables++;
	vl->variables = (ShVariable**) realloc(vl->variables,
			vl->numVariables * sizeof(ShVariable*));
	vl->variables[vl->numVariables - 1] = v;
}

ShVariable* findShVariableFromId(ShVariableList *vl, int id)
{
	ShVariable **vp = NULL;
	int i;

	if (!vl) {
		return NULL;
	}

	vp = vl->variables;

	for (i = 0; i < vl->numVariables; i++) {
		if (vp[i]->uniqueId == id) {
			return vp[i];
		}
	}

	return NULL;
}

char* ShGetTypeString(const ShVariable *v)
{
	char *result;

	if (!v)
		return NULL;

	if (v->isArray) {
		if (v->isMatrix) {
			asprintf(&result, "array of mat[%i][%i]", v->matrixSize[0],
					v->matrixSize[1]);
		} else if (v->size != 1) {
			switch (v->type) {
			case SH_FLOAT:
				asprintf(&result, "array of vec%i", v->size);
				break;
			case SH_INT:
				asprintf(&result, "array of ivec%i", v->size);
				break;
			case SH_UINT:
				asprintf(&result, "array of uvec%i", v->size);
				break;
			case SH_BOOL:
				asprintf(&result, "array of bvec%i", v->size);
				break;
			case SH_STRUCT:
				asprintf(&result, "array of %s", v->structName);
				break;
			default:
				asprintf(&result, "unknown type");
				return result;
			}
		} else {
			asprintf(&result, "array of %s", getShTypeString((ShVariable*) v));
			return result;
		}
	} else {
		if (v->isMatrix) {
			asprintf(&result, "mat[%i][%i]", v->matrixSize[0],
					v->matrixSize[1]);
		} else if (v->size != 1) {
			switch (v->type) {
			case SH_FLOAT:
				asprintf(&result, "vec%i", v->size);
				break;
			case SH_INT:
				asprintf(&result, "ivec%i", v->size);
				break;
			case SH_UINT:
				asprintf(&result, "uvec%i", v->size);
				break;
			case SH_BOOL:
				asprintf(&result, "bvec%i", v->size);
				break;
			case SH_STRUCT:
				asprintf(&result, "%s", v->structName);
				break;
			default:
				asprintf(&result, "unknown type");
				return result;
			}
		} else {
			asprintf(&result, getShTypeString((ShVariable*) v));
			return result;
		}

	}
	return result;
}

const char* ShGetQualifierString(const ShVariable *v)
{
	switch (v->qualifier) {
	case SH_UNSET:
	case SH_TEMPORARY:
	case SH_GLOBAL:
		return "";
	case SH_CONST:
		return "const";
	case SH_ATTRIBUTE:
		return "attribute";
	case SH_VARYING_IN:
		return "varying in";
	case SH_VARYING_OUT:
		return "varying out";
	case SH_UNIFORM:
		return "uniform";
	case SH_PARAM_IN:
		return "in parameter";
	case SH_PARAM_OUT:
		return "out parameter";
	case SH_PARAM_INOUT:
		return "inout parameter";
	case SH_PARAM_CONST:
		return "const parameter";
	case SH_BUILTIN_READ:
		return "builtin read";
	case SH_BUILTIN_WRITE:
		return "builtin write";
	default:
		return "unknown qualifier";

	}
}

ShVariable* copyShVariable(ShVariable *src)
{
	ShVariable *ret;
	int i;

	if (!src)
		return NULL;

	if (!(ret = (ShVariable*) malloc(sizeof(ShVariable)))) {
		dbgPrint(DBGLVL_ERROR, "not enough memory to copy ShVariable\n");
		exit(1);
	}

	ret->uniqueId = src->uniqueId;
	ret->builtin = src->builtin;

	if (src->name) {
		if (!(ret->name = (char*) malloc(strlen(src->name) + 1))) {
			dbgPrint(DBGLVL_ERROR,
					"not enough memory to copy name of ShVariable\n");
			exit(1);
		}
		strcpy(ret->name, src->name);
	} else {
		ret->name = NULL;
	}

	ret->type = src->type;
	ret->qualifier = src->qualifier;
	ret->size = src->size;
	ret->isMatrix = src->isMatrix;
	ret->matrixSize[0] = src->matrixSize[0];
	ret->matrixSize[1] = src->matrixSize[1];
	ret->isArray = src->isArray;
	for (i = 0; i < MAX_ARRAYS; i++) {
		ret->arraySize[i] = src->arraySize[i];
	}

	if (src->structName) {
		if (!(ret->structName = (char*) malloc(strlen(src->structName) + 1))) {
			dbgPrint(DBGLVL_ERROR,
					"not enough memory to copy strctName of ShVariable\n");
			exit(1);
		}
		strcpy(ret->structName, src->structName);
	} else {
		ret->structName = NULL;
	}

	ret->structSize = src->structSize;

	if (!(ret->structSpec = (ShVariable**) malloc(
			sizeof(ShVariable*) * ret->structSize))) {
		dbgPrint(DBGLVL_ERROR,
				"not enough memory to copy structSpec of ShVariable\n");
		exit(1);
	}
	for (i = 0; i < ret->structSize; i++) {
		ret->structSpec[i] = copyShVariable(src->structSpec[i]);
	}

	return ret;
}

void freeShVariable(ShVariable **var)
{
	if (var && *var) {
		// Remove variable from storages
		std::map<int, ShVariable*>::iterator iit = VariablesById.find(
				(*var)->uniqueId);
		if (iit != VariablesById.end())
			VariablesById.erase(iit);

		for (VariableMap::iterator it = VariablesBySource.begin(), end = VariablesBySource.end();
				it != end; ++it) {
			if (it->second != *var)
				continue;
			it = VariablesBySource.erase(it);
			break;
		}

		int i;
		free((*var)->name);
		for (i = 0; i < (*var)->structSize; i++) {
			freeShVariable(&(*var)->structSpec[i]);
		}
		free((*var)->structSpec);
		free((*var)->structName);
		free(*var);
		*var = NULL;
	}
}

void freeShVariableList(ShVariableList *vl)
{
	int i;
	for (i = 0; i < vl->numVariables; i++) {
		freeShVariable(&vl->variables[i]);
	}
	free(vl->variables);
	vl->numVariables = 0;
}

ShVariable* glsltypeToShVariable(const struct glsl_type* vtype,
		const char* name, variableQualifier qualifier, unsigned modifier =
				SH_VM_NONE)
{
	ShVariable *v = NULL;
	v = (ShVariable*) malloc(sizeof(ShVariable));

	// Type has no identifier! To be filled in later by TVariable
	v->uniqueId = -1;
	v->builtin = false;
	v->name = strdup(name);

#define SET_TYPE(glsl, native) \
    case glsl: \
        v->type = native; \
        break;

	// Type of variable (SH_FLOAT/SH_INT/SH_BOOL/SH_STRUCT)
	switch (vtype->base_type) {
	SET_TYPE( GLSL_TYPE_UINT, SH_UINT)
	SET_TYPE( GLSL_TYPE_INT, SH_INT)
	SET_TYPE( GLSL_TYPE_FLOAT, SH_FLOAT)
	SET_TYPE( GLSL_TYPE_BOOL, SH_BOOL)
	SET_TYPE( GLSL_TYPE_SAMPLER, SH_SAMPLER_GUARD_BEGIN)
	SET_TYPE( GLSL_TYPE_STRUCT, SH_STRUCT)
	SET_TYPE( GLSL_TYPE_ARRAY, SH_ARRAY)
	default:
		dbgPrint( DBGLVL_ERROR, "Type does not defined %x", vtype->gl_type);
		break;
	}

#undef SET_TYPE

	// Qualifier of variable
	v->qualifier = qualifier;

	// Varying modifier
	v->varyingModifier = modifier;

	// Scalar/Vector size
	v->size = vtype->components();

	// Matrix handling
	v->isMatrix = vtype->is_matrix();
	v->matrixSize[0] = vtype->vector_elements;
	v->matrixSize[1] = vtype->matrix_columns;

	// Array handling
	v->isArray = vtype->is_array();
	for (int i = 0; i < MAX_ARRAYS; i++) {
		v->arraySize[i] = vtype->array_size();
	}

	if (vtype->base_type == GLSL_TYPE_STRUCT) {
		//
		// Append structure to ShVariable
		//
		v->structSize = vtype->length;
		v->structSpec = (ShVariable**) malloc(
				v->structSize * sizeof(ShVariable*));
		v->structName = strdup(vtype->name);
		for (int i = 0; i < v->structSize; ++i) {
			struct glsl_struct_field* field = &vtype->fields.structure[i];
			v->structSpec[i] = glsltypeToShVariable(field->type, field->name,
					qualifier);
		}
	} else {
		v->structName = NULL;
		v->structSize = 0;
		v->structSpec = NULL;
	}

	return v;
}



int addShVariables(ShVariableList* vl, ast_declarator_list* list, const struct glsl_type * decl_type)
{
	int count = 0;

	foreach_list_typed (ast_declaration, decl, link, &list->declarations) {
		ShVariable *var = NULL;
		VariableMap::iterator it = VariablesBySource.find(decl);
		if (it != VariablesBySource.end()){
			var = it->second;
		} else {
			variableQualifier qualifier = qualifierToNative(&list->type->qualifier, false);
			unsigned modifier = 0;
			modifier |= list->type->qualifier.flags.q.invariant * SH_VM_INVARIANT;
			modifier |=	list->type->qualifier.flags.q.flat * SH_VM_FLAT;
			modifier |= list->type->qualifier.flags.q.smooth * SH_VM_SMOOTH;
			modifier |= list->type->qualifier.flags.q.noperspective * SH_VM_NOPERSPECTIVE;
			modifier |= list->type->qualifier.flags.q.centroid * SH_VM_CENTROID;
			var = glsltypeToShVariable(decl_type, decl->identifier, qualifier, modifier);
			var->uniqueId = VariablesCount++;
			VariablesById[var->uniqueId] = var;
			VariablesBySource[decl] = var;
		}

		addShVariable(vl, var, strncmp(decl->identifier, "gl_", 3 ) == 0);
		count++;
	}

	return count;
}

void ShDumpVariable(ShVariable *v, int depth)
{
	int i;

	for (i = 0; i < depth; i++)
		dbgPrint(DBGLVL_COMPILERINFO, "    ");

	if (0 <= v->uniqueId) {
		dbgPrint(DBGLVL_COMPILERINFO, "<%i> ", v->uniqueId);
	}

	if (v->builtin) {
		dbgPrint(DBGLVL_COMPILERINFO, "builtin ");
	}

	dbgPrint(DBGLVL_COMPILERINFO,
			"%s %s", getShQualifierString(v->qualifier), getShTypeString(v));

	if (v->isMatrix) {
		dbgPrint(DBGLVL_COMPILERINFO, "%ix%i", v->size, v->size);
	} else {
		if (1 < v->size) {
			dbgPrint(DBGLVL_COMPILERINFO, "%i", v->size);
		}
	}

	dbgPrint(DBGLVL_COMPILERINFO, " %s", v->name);

	if (v->isArray) {
		for (i = 0; i < MAX_ARRAYS; i++) {
			if (v->arraySize[i] != -1) {
				dbgPrint(DBGLVL_COMPILERINFO, "[%i]", v->arraySize[i]);
			} else {
				break;
			}
		}
		dbgPrint(DBGLVL_COMPILERINFO, "\n");
	} else {
		dbgPrint(DBGLVL_COMPILERINFO, "\n");
	}

	if (v->structSize != 0) {
		depth++;
		for (i = 0; i < v->structSize; i++) {
			ShDumpVariable(v->structSpec[i], depth);
		}
	}

}

ShVariable* findFirstShVariableFromName(ShVariableList *vl, const char *name)
{
	ShVariable **vp = NULL;
	int i;

	if (!vl) {
		return NULL;
	}

	vp = vl->variables;

	for (i = 0; i < vl->numVariables; i++) {
		if (!(strcmp(vp[i]->name, name))) {
			return vp[i];
		}
	}
	return NULL;
}

ShVariable* findShVariableFromSource(ast_declaration* variable)
{
	VariableMap::iterator it = VariablesBySource.find(variable);
	if (it != VariablesBySource.end())
		return it->second;
	return NULL;
}