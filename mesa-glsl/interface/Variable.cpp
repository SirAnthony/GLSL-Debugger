/*
 * Variable.cpp
 *
 *  Created on: 23.02.2014
 */

#include "ShaderLang.h"
#include "AstScope.h"
#include "mesa/glsl/ast.h"
#include "glsldb/utils/dbgprint.h"
#include <map>


namespace {
	const char* identifierFromNode(ast_node* decl)
	{
		ast_declaration* as_decl = decl->as_declaration();
		ast_parameter_declarator* as_param = decl->as_parameter_declarator();
		ast_struct_specifier* as_struct = decl->as_struct_specifier();
		ast_expression* as_expr = decl->as_expression();

		if (as_decl)
			return as_decl->identifier;
		else if (as_param)
			return as_param->identifier;
		else if (as_struct)
			return as_struct->name;
		else if (as_expr && as_expr->oper == ast_identifier)
			return as_expr->primary_expression.identifier;
		else
			return NULL;
	}

	std::map<int, ShVariable*> VariablesById;
	unsigned int VariablesCount = 0;
}


static const char* getShTypeString(const ShVariable *v)
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
	case SH_SAMPLER:
		return "sampler";
	case SH_IMAGE:
		return "image";
	case SH_ATOMIC:
		return "atomic";
	case SH_INTERFACE:
		return "interface";
	case SH_VOID:
		return "void";
	default:
		return "unknown";
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


variableQualifier qualifierFromAst(const ast_type_qualifier* qualifier, bool is_parameter)
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
}

variableQualifier qualifierFromIr(ir_variable* var)
{
//	TODO: not sure about origin
//	SH_CONST,
//	SH_ATTRIBUTE,

	switch (var->data.mode) {
	case ir_var_uniform:
		return SH_UNIFORM;
	case ir_var_shader_in:
		return SH_VARYING_IN;
	case ir_var_shader_out:
		return SH_VARYING_OUT;
	case ir_var_function_in:
		return SH_PARAM_IN;
	case ir_var_function_out:
		return SH_PARAM_OUT;
	case ir_var_function_inout:
		return SH_PARAM_INOUT;
	case ir_var_const_in: /**< "in" param that must be a constant expression */
		return SH_PARAM_CONST;
	case ir_var_system_value: /**< Ex: front-face, instance-id, etc. */
		if (var->data.read_only)
			return SH_BUILTIN_READ;
		else
			return SH_BUILTIN_WRITE;
	case ir_var_temporary: /**< Temporary variable generated during compilation. */
		return SH_TEMPORARY;
	default:
		return SH_GLOBAL;
	}
	return SH_GLOBAL;
}

variableVaryingModifier modifierFromAst(const ast_type_qualifier* qualifier)
{
	variableVaryingModifier mod = SH_VM_NONE;
	mod |= qualifier->flags.q.invariant * SH_VM_INVARIANT;
	mod |= qualifier->flags.q.flat * SH_VM_FLAT;
	mod |= qualifier->flags.q.smooth * SH_VM_SMOOTH;
	mod |= qualifier->flags.q.noperspective * SH_VM_NOPERSPECTIVE;
	mod |= qualifier->flags.q.centroid * SH_VM_CENTROID;
	return mod;
}

variableVaryingModifier modifierFromIr(const ir_variable* var)
{
	variableVaryingModifier mod = SH_VM_NONE;
	mod |= var->data.invariant * SH_VM_INVARIANT;
	mod |= var->data.centroid * SH_VM_CENTROID;

	switch (var->data.interpolation) {
	case INTERP_QUALIFIER_SMOOTH:
		mod |= SH_VM_SMOOTH;
		break;
	case INTERP_QUALIFIER_FLAT:
		mod |= SH_VM_FLAT;
		break;
	case INTERP_QUALIFIER_NOPERSPECTIVE:
		mod |= SH_VM_NOPERSPECTIVE;
		break;
	default:
		break;
	}

	return mod;
}

variableVaryingModifier modifierFromGLSL(struct glsl_struct_field* field, int invariant)
{
	variableVaryingModifier mod = SH_VM_NONE;
	mod |= invariant;
	mod |= field->centroid * SH_VM_CENTROID;

	switch (field->interpolation) {
	case INTERP_QUALIFIER_SMOOTH:
		mod |= SH_VM_SMOOTH;
		break;
	case INTERP_QUALIFIER_FLAT:
		mod |= SH_VM_FLAT;
		break;
	case INTERP_QUALIFIER_NOPERSPECTIVE:
		mod |= SH_VM_NOPERSPECTIVE;
		break;
	default:
		break;
	}

	return mod;
}

void addShVariableCtx(ShVariableList *vl, ShVariable *v, int builtin, void* mem_ctx)
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
	vl->variables = (ShVariable**) reralloc_array_size(mem_ctx, vl->variables,
			sizeof(ShVariable*), vl->numVariables);
	vl->variables[vl->numVariables - 1] = v;
}

ShVariable* findShVariable(int id)
{
	if (id >= 0) {
		std::map<int, ShVariable*>::iterator it = VariablesById.find(id);
		if (it != VariablesById.end())
			return it->second;
	}
	return NULL;
}

ShVariable* findShVariableFromId(ShVariableList *vl, int id)
{
	ShVariable **vp = NULL;
	int i;

	if (!vl)
		return NULL;

	vp = vl->variables;
	for (i = 0; i < vl->numVariables; i++) {
		if (vp[i]->uniqueId == id)
			return vp[i];
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
			asprintf(&result, "array of mat[%i][%i]", v->size / v->matrixColumns,
					v->matrixColumns);
		} else if (v->size != 1) {
			switch (v->arrayType) {
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
			asprintf(&result, "array of %s", getShTypeString(v));
			return result;
		}
	} else {
		if (v->isMatrix) {
			asprintf(&result, "mat[%i][%i]", v->size / v->matrixColumns,
					v->matrixColumns);
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

ShVariable* copyShVariableCtx(ShVariable *src, void* mem_ctx)
{
	if (!src)
		return NULL;

	ShVariable* ret = rzalloc(mem_ctx, ShVariable);
	assert(ret || !"not enough memory to copy ShVariable");

	ret->uniqueId = src->uniqueId;
	ret->builtin = src->builtin;
	if (src->name)
		ret->name = ralloc_strdup(ret, src->name);

	ret->type = src->type;
	ret->gl_type = src->gl_type;
	ret->qualifier = src->qualifier;
	memcpy(&ret->sampler, &src->sampler, sizeof(ShVariable::samplerInfo));
	ret->size = src->size;
	ret->isMatrix = src->isMatrix;
	ret->matrixColumns = src->matrixColumns;
	ret->isArray = src->isArray;
	if (ret->isArray) {
		ret->arrayDepth = src->arrayDepth;
		ret->arraySize = rzalloc_array(ret, int, src->arrayDepth);
		memcpy(ret->arraySize, src->arraySize, ret->arrayDepth);
	}

	if (src->structName)
		ret->structName = ralloc_strdup(ret, src->structName);

	ret->structSize = src->structSize;
	if (ret->structSize) {
		ret->structSpec = rzalloc_array(ret, ShVariable*, ret->structSize);
		assert(ret->structSpec || !"not enough memory to copy structSpec of ShVariable");
		for (int i = 0; i < ret->structSize; i++)
			ret->structSpec[i] = copyShVariableCtx(src->structSpec[i], ret->structSpec);
	}

	return ret;
}



ShVariable* copyShVariable(ShVariable *src)
{
	dbgPrint(DBGLVL_INTERNAL_WARNING, "Copy of ShVariable out of shader mem.\n");
	return copyShVariableCtx(src, NULL);
}

void freeShVariable(ShVariable **var)
{
	if (!var || *var == NULL)
		return;

	// Remove variable from storages if it is not copy
	auto iit = VariablesById.find((*var)->uniqueId);
	if (iit != VariablesById.end() && iit->second == *var)
		VariablesById.erase(iit);

	ralloc_free(*var);
	*var = NULL;
}

void freeShVariableList(ShVariableList *vl)
{
	for (int i = 0; i < vl->numVariables; i++)
		freeShVariable(&vl->variables[i]);
	ralloc_free(vl->variables);
	vl->numVariables = 0;
}

static variableType typeFromGlsl(const struct glsl_type* type)
{
	switch (type->base_type) {
	case GLSL_TYPE_UINT:
		return SH_UINT;
	case GLSL_TYPE_INT:
		return SH_INT;
	case GLSL_TYPE_FLOAT:
		return SH_FLOAT;
	case GLSL_TYPE_BOOL:
		return SH_BOOL;
	case GLSL_TYPE_SAMPLER:
		return SH_SAMPLER;
	case GLSL_TYPE_IMAGE:
		return SH_IMAGE;
	case GLSL_TYPE_ATOMIC_UINT:
		return SH_ATOMIC;
	case GLSL_TYPE_STRUCT:
		return SH_STRUCT;
	case GLSL_TYPE_INTERFACE:
		return SH_INTERFACE;
	case GLSL_TYPE_ARRAY:
		return SH_ARRAY;
	case GLSL_TYPE_VOID:
		return SH_VOID;
	}
	dbgPrint(DBGLVL_ERROR, "Type does not defined %x", type->base_type);
	return SH_ERROR;
}


static void glsltypeToShVariable(ShVariable* var, const struct glsl_type* vtype,
		variableQualifier qualifier, variableVaryingModifier modifier)
{

	var->type = typeFromGlsl(vtype);
	var->gl_type = vtype->gl_type;

	// Qualifier of variable
	var->qualifier = qualifier;

	// Varying modifier
	var->varyingModifier = modifier;

	// Scalar/Vector size
	var->size = vtype->components();

	// Matrix handling
	var->isMatrix = vtype->is_matrix();
	var->matrixColumns = vtype->matrix_columns;

	var->sampler.array = vtype->sampler_array;
	var->sampler.dimensionality = vtype->sampler_dimensionality;
	var->sampler.shadow = vtype->sampler_shadow;
	var->sampler.type = vtype->sampler_type;

	// Array handling
	var->isArray = vtype->is_array();
	if (var->isArray) {
		var->arrayDepth = 1;
		var->arrayType = typeFromGlsl(vtype->fields.array);
		const glsl_type* atype = vtype;
		while (atype->is_array()) {
			var->arraySize = reralloc(var, var->arraySize, int, var->arrayDepth);
			var->arraySize[var->arrayDepth++] = atype->length;
			atype = atype->fields.array;
		}
	}

	if (vtype->base_type == GLSL_TYPE_STRUCT) {
		// Add variables recursive
		var->structSize = vtype->length;
		var->structSpec = (ShVariable**) ralloc_array(var, ShVariable*, var->structSize);
		var->structName = ralloc_strdup(var, vtype->name);
		for (int i = 0; i < var->structSize; ++i) {
			struct glsl_struct_field* field = &vtype->fields.structure[i];
			ShVariable* svar = rzalloc(var->structSpec, ShVariable);
			svar->uniqueId = -1;
			if (qualifier & SH_BUILTIN)
				svar->qualifier = SH_BUILTIN;
			svar->name = ralloc_strdup(svar, field->name);
			int field_mod = modifierFromGLSL(field, modifier & SH_VM_INVARIANT);
			glsltypeToShVariable(svar, field->type, qualifier, field_mod);
			var->structSpec[i] = svar;
		}
	} else {
		var->structName = NULL;
		var->structSize = 0;
		var->structSpec = NULL;
	}

#undef SET_TYPE
}

void addAstShVariable(ast_node* decl, ShVariable* var)
{
	var->uniqueId = VariablesCount++;
	VariablesById[var->uniqueId] = var;
	decl->debug_id = var->uniqueId;
}


ShVariable* astToShVariable(ast_node* decl, variableQualifier qualifier,
		variableVaryingModifier modifier, const struct glsl_type* decl_type,
		void* mem_ctx)
{
	if (!decl)
		return NULL;

	const char* identifier = identifierFromNode(decl);
	if (!identifier)
		return NULL;

	ShVariable* var = findShVariable(decl->debug_id);
	if (!var) {
		var = rzalloc(mem_ctx, ShVariable);
		var->uniqueId = -1;
		var->builtin = qualifier & SH_BUILTIN;
		var->name = ralloc_strdup(var, identifier);
		glsltypeToShVariable(var, decl_type, qualifier, modifier);
		addAstShVariable(decl, var);
	}

	return var;
}


void ShDumpVariable(ShVariable *v, int depth)
{
	for (int i = 0; i < depth; i++)
		dbgPrint(DBGLVL_COMPILERINFO, "    ");

	if (0 <= v->uniqueId)
		dbgPrint(DBGLVL_COMPILERINFO, "<%i> ", v->uniqueId);

	if (v->builtin)
		dbgPrint(DBGLVL_COMPILERINFO, "builtin ");

	dbgPrint(DBGLVL_COMPILERINFO,
			"%s %s", getShQualifierString(v->qualifier), getShTypeString(v));

	if (v->isMatrix) {
		dbgPrint(DBGLVL_COMPILERINFO, "%ix%i", v->size, v->size);
	} else {
		if (1 < v->size)
			dbgPrint(DBGLVL_COMPILERINFO, "%i", v->size);
	}

	dbgPrint(DBGLVL_COMPILERINFO, " %s", v->name);

	if (v->isArray) {
		for (int i = 0; i < v->arrayDepth; i++) {
			if (v->arraySize[i] < 0)
				break;
			dbgPrint(DBGLVL_COMPILERINFO, "[%i]", v->arraySize[i]);
		}
		dbgPrint(DBGLVL_COMPILERINFO, "\n");
	} else {
		dbgPrint(DBGLVL_COMPILERINFO, "\n");
	}

	if (v->structSize != 0) {
		depth++;
		for (int i = 0; i < v->structSize; i++)
			ShDumpVariable(v->structSpec[i], depth);
	}

}


ShVariable* findFirstShVariableFromName(ShVariableList *vl, const char *name)
{
	ShVariable **vp = NULL;
	int i;

	if (!vl)
		return NULL;

	vp = vl->variables;
	for (i = 0; i < vl->numVariables; i++) {
		if (!(strcmp(vp[i]->name, name)))
			return vp[i];
	}

	return NULL;
}
