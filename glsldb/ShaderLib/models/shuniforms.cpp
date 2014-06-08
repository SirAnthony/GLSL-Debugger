
#include "shuniforms.h"
#include <QtOpenGL>
#include "utils/dbgprint.h"

ShUniform::ShUniform(const char *serializedUniform, int &length)
{
	GLint name_length, value_size, array_size;
	GLuint gltype;
	length = 0;

	memcpy(&name_length, serializedUniform, sizeof(GLint));
	length += sizeof(GLint);
	uniformName = QString::fromAscii(serializedUniform + length, name_length);
	length += name_length;
	memcpy(&gltype, serializedUniform + length, sizeof(GLuint));
	length += sizeof(GLuint);
	memcpy(&array_size, serializedUniform + length, sizeof(GLint));
	arraySize = array_size;
	length += sizeof(GLint);
	memcpy(&value_size, serializedUniform + length, sizeof(GLint));
	length += sizeof(GLint);

	type = shuDefault;
	switch (gltype) {
	case GL_FLOAT:
		data = new TypedShUniformData<GLfloat>(serializedUniform + length, 1);
		arraySize = 1;
		matrixColumns = 1;
		break;
	case GL_FLOAT_VEC2:
		data = new TypedShUniformData<GLfloat>(serializedUniform + length, 2);
		type = shuVector;
		arraySize = 2;
		matrixColumns = 1;
		break;
	case GL_FLOAT_VEC3:
		data = new TypedShUniformData<GLfloat>(serializedUniform + length, 3);
		type = shuVector;
		arraySize = 3;
		matrixColumns = 1;
		break;
	case GL_FLOAT_VEC4:
		data = new TypedShUniformData<GLfloat>(serializedUniform + length, 4);
		type = shuVector;
		matrixColumns = 1;
		arraySize = 4;
		break;
	case GL_FLOAT_MAT2:
		data = new TypedShUniformData<GLfloat>(serializedUniform + length, 4);
		type = shuMatrix;
		matrixColumns = 2;
		arraySize = 2;
		break;
	case GL_FLOAT_MAT2x3:
		data = new TypedShUniformData<GLfloat>(serializedUniform + length, 6);
		type = shuMatrix;
		matrixColumns = 2;
		arraySize = 3;
		break;
	case GL_FLOAT_MAT2x4:
		data = new TypedShUniformData<GLfloat>(serializedUniform + length, 8);
		type = shuMatrix;
		matrixColumns = 2;
		arraySize = 4;
		break;
	case GL_FLOAT_MAT3:
		data = new TypedShUniformData<GLfloat>(serializedUniform + length, 9);
		type = shuMatrix;
		matrixColumns = 3;
		arraySize = 3;
		break;
	case GL_FLOAT_MAT3x2:
		data = new TypedShUniformData<GLfloat>(serializedUniform + length, 6);
		type = shuMatrix;
		matrixColumns = 3;
		arraySize = 2;
		break;
	case GL_FLOAT_MAT3x4:
		data = new TypedShUniformData<GLfloat>(serializedUniform + length, 12);
		type = shuMatrix;
		matrixColumns = 3;
		arraySize = 4;
		break;
	case GL_FLOAT_MAT4:
		data = new TypedShUniformData<GLfloat>(serializedUniform + length, 16);
		type = shuMatrix;
		matrixColumns = 4;
		arraySize = 4;
		break;
	case GL_FLOAT_MAT4x2:
		data = new TypedShUniformData<GLfloat>(serializedUniform + length, 8);
		type = shuMatrix;
		matrixColumns = 4;
		arraySize = 2;
		break;
	case GL_FLOAT_MAT4x3:
		data = new TypedShUniformData<GLfloat>(serializedUniform + length, 12);
		type = shuMatrix;
		matrixColumns = 4;
		arraySize = 3;
		break;
	case GL_INT:
		data = new TypedShUniformData<GLint>(serializedUniform + length, 1);
		matrixColumns = 1;
		arraySize = 1;
		break;
	case GL_INT_VEC2:
		data = new TypedShUniformData<GLint>(serializedUniform + length, 2);
		type = shuVector;
		matrixColumns = 1;
		arraySize = 2;
		break;
	case GL_INT_VEC3:
		data = new TypedShUniformData<GLint>(serializedUniform + length, 3);
		type = shuVector;
		matrixColumns = 1;
		arraySize = 3;
		break;
	case GL_INT_VEC4:
		data = new TypedShUniformData<GLint>(serializedUniform + length, 4);
		type = shuVector;
		matrixColumns = 1;
		arraySize = 4;
		break;
	case GL_SAMPLER_1D:
	case GL_SAMPLER_2D:
	case GL_SAMPLER_3D:
	case GL_SAMPLER_CUBE:
	case GL_SAMPLER_1D_SHADOW:
	case GL_SAMPLER_2D_SHADOW:
	case GL_SAMPLER_2D_RECT_ARB:
	case GL_SAMPLER_2D_RECT_SHADOW_ARB:
	case GL_SAMPLER_1D_ARRAY_EXT:
	case GL_SAMPLER_2D_ARRAY_EXT:
	case GL_SAMPLER_BUFFER_EXT:
	case GL_SAMPLER_1D_ARRAY_SHADOW_EXT:
	case GL_SAMPLER_2D_ARRAY_SHADOW_EXT:
	case GL_SAMPLER_CUBE_SHADOW_EXT:
	case GL_INT_SAMPLER_1D_EXT:
	case GL_INT_SAMPLER_2D_EXT:
	case GL_INT_SAMPLER_3D_EXT:
	case GL_INT_SAMPLER_CUBE_EXT:
	case GL_INT_SAMPLER_2D_RECT_EXT:
	case GL_INT_SAMPLER_1D_ARRAY_EXT:
	case GL_INT_SAMPLER_2D_ARRAY_EXT:
	case GL_INT_SAMPLER_BUFFER_EXT:
	case GL_UNSIGNED_INT_SAMPLER_1D_EXT:
	case GL_UNSIGNED_INT_SAMPLER_2D_EXT:
	case GL_UNSIGNED_INT_SAMPLER_3D_EXT:
	case GL_UNSIGNED_INT_SAMPLER_CUBE_EXT:
	case GL_UNSIGNED_INT_SAMPLER_2D_RECT_EXT:
	case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY_EXT:
	case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY_EXT:
	case GL_UNSIGNED_INT_SAMPLER_BUFFER_EXT:
		data = new TypedShUniformData<GLint>(serializedUniform + length, 1);
		matrixColumns = 1;
		arraySize = 1;
		break;
	case GL_BOOL:
		data = new TypedShUniformData<GLboolean>(serializedUniform + length, 1);
		matrixColumns = 1;
		arraySize = 1;
		break;
	case GL_BOOL_VEC2:
		data = new TypedShUniformData<GLboolean>(serializedUniform + length, 2);
		type = shuVector;
		matrixColumns = 1;
		arraySize = 2;
		break;
	case GL_BOOL_VEC3:
		data = new TypedShUniformData<GLboolean>(serializedUniform + length, 3);
		type = shuVector;
		matrixColumns = 1;
		arraySize = 3;
		break;
	case GL_BOOL_VEC4:
		data = new TypedShUniformData<GLboolean>(serializedUniform + length, 4);
		type = shuVector;
		matrixColumns = 1;
		arraySize = 4;
		break;
	case GL_UNSIGNED_INT:
		data = new TypedShUniformData<GLuint>(serializedUniform + length,	1);
		matrixColumns = 1;
		arraySize = 1;
		break;
	case GL_UNSIGNED_INT_VEC2_EXT:
		data = new TypedShUniformData<GLuint>(serializedUniform + length,	2);
		type = shuVector;
		matrixColumns = 1;
		arraySize = 2;
		break;
	case GL_UNSIGNED_INT_VEC3_EXT:
		data = new TypedShUniformData<GLuint>(serializedUniform + length,	3);
		type = shuVector;
		matrixColumns = 1;
		arraySize = 3;
		break;
	case GL_UNSIGNED_INT_VEC4_EXT:
		data = new TypedShUniformData<GLuint>(serializedUniform + length,	4);
		type = shuVector;
		matrixColumns = 1;
		arraySize = 4;
		break;
	default:
		dbgPrint(DBGLVL_ERROR, "HMM, unkown shader variable type: %i\n", gltype);
		data = NULL;
	}

	dbgPrint(DBGLVL_INFO, "Got Uniform " "%s" ":\n", uniformName.toAscii().data());
	dbgPrint(DBGLVL_INFO, "    size: %d\n", arraySize);
	dbgPrint(DBGLVL_INFO, "    isVector: %d\n", type == shuVector);
	dbgPrint(DBGLVL_INFO, "    isMatrix: %d\n", type == shuMatrix);
	dbgPrint(DBGLVL_INFO, "    matrixColumns: %d\n", matrixColumns);

	length += value_size;
}

ShUniform::~ShUniform()
{

}

QString ShUniform::toString() const
{
	QString result;

	for (int idx = 0; idx < arraySize; ++idx) {
		if (idx)
			result += ", ";
		result += toString(idx);
	}

	return result;
}

QString ShUniform::toString(int index) const
{
	if (index > arraySize)
		return "?";


	QString result;
	if (type == shuMatrix)
		result += '(';

	bool bracket = (type == shuMatrix || type == shuVector);
	for (int j = 0; j < arraySize; ++j) {
		if (j)
			result += ", ";
		if (bracket)
			result += '(';

		for (int i = 0; i < matrixColumns; ++i) {
			if (i)
				result += ", ";
			result += data->toString(matrixColumns * (index * arraySize + j) + i);
		}

		if (bracket)
			result += ')';
	}

	if (type == shuMatrix)
		result += ')';
	return result;
}
