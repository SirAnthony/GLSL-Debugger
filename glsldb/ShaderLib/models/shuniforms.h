
#ifndef SHMODEL_UNIFORMS_H_
#define SHMODEL_UNIFORMS_H_

#include <QString>
#include <string.h>

class ShUniformData {
public:
	virtual ~ShUniformData() {}
	virtual QString toString(int element) const = 0;
};

template<typename T>
class TypedShUniformData: public ShUniformData {
public:
	TypedShUniformData(const char * const _data, int count)
	{
		data = new T[count];
		memcpy(data, _data, sizeof(T) * count);
		elements = count;
	}

	~TypedShUniformData()
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
	{ return uniformName; }

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
	QString uniformName;
	ShUniformType type;
	int arraySize;
	int matrixColumns;
	ShUniformData* data;
};




#endif /* SHMODEL_UNIFORMS_H_ */
