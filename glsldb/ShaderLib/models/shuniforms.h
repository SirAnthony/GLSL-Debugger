
#ifndef SHMODEL_UNIFORMS_H_
#define SHMODEL_UNIFORMS_H_

#include <QString>
#include <string.h>

class ShUniformData {
public:
	virtual ~UniformData() {}
	virtual QString toString(int element) const = 0;
};

template<typename T>
class TypedShUniformData: public ShUniformData {
public:
	TypedUniformData(const char * const data, int count)
	{
		data = new T[count];
		memcpy(data, data, sizeof(T) * count);
		elements = count;
	}

	~TypedUniformData()
	{
		delete[] data;
	}

	QString toString(int element) const
	{
		if (element < elements)
			return QString::number(data[element]);
		return "?";
	}

private:
	T* data;
	int elements;
};

enum ShUniformType
{
	shuDefault = 0,
	shuVector,
	shuMatrix
};

class ShUniform {
public:

	ShUniform(const char* serializedUniform, int &length);
	~ShUniform();

	QString name() const
	{ return name; }

	bool isVector() const
	{ return type == shuVector; }

	bool isMatrix() const
	{ return type == shuMatrix; }

	int rows() const
	{ return arraySize; }

	int columns() const
	{ return matrixColumns; }

	int size() const
	{return arraySize * matrixColumns; }

	QString toString() const;
	QString toString(int index) const;

private:
	QString name;
	ShUniformType type;
	int arraySize;
	int matrixColumns;
	ShUniformData* data;
};




#endif /* SHMODEL_UNIFORMS_H_ */
