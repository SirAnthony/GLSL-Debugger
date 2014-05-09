
#ifndef GLSCATTER_H
#define GLSCATTER_H

#include <GL/gl.h>
#include <GL/glext.h>
#include <QGLWidget>
#include <QGLFunctions>
#include <QQuaternion>
#include <QVector3D>
#include <QMatrix4x4>
#include <QGLShaderProgram>


class GLScatter: public QGLWidget, protected QGLFunctions {
Q_OBJECT

public:
	GLScatter(QWidget *parent = 0);
	~GLScatter();

	void setClearColor(QColor c);
	QColor getClearColor(void)
	{
		return clearColor;
	}

	void setPointSize(float ps)
	{
		psize = ps;
	}
	float getPointSize()
	{
		return psize;
	}

	void resetView(void);

	void setData(float *positions, float *colors, int points, int elements);

protected:
	void initializeGL();
	void initShaders();
	void paintGL();
	void resizeGL(int width, int height);
	void drawAxes(QMatrix4x4 &matrix);
	void drawArray(QGLShaderProgram *prog, QMatrix4x4 &mvp, GLenum type, int components,
				   GLuint vbo_pos, size_t size_pos, int count_pos, int offset_pos,
				   GLuint vbo_color = 0, size_t size_color = 0, int count_color = 0,
				   int offset_color = 0);

	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);

private:
	enum VBO_ID {
		vboAxes, vboAxesBox,
		vboDataVertex, vboDataColor,
		vboLast
	};
	enum PROGRAM_ID {
		progAxes, progAxesBox, progData, progLast
	};

	QGLShaderProgram program[progLast];
	GLuint vbos[vboLast];
	int points;
	int elements;

	QMatrix4x4 projection;
	QQuaternion rotation;
	QVector3D translation;
	float camDist;

	QPoint lastMousePos;
	QColor clearColor;
	float psize;
};

#endif /* GLSCATTER_H */

