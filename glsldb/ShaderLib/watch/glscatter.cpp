
#include "glscatter.h"
#include <QtCore/QFile>
#include <QtGui/QMouseEvent>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "utils/dbgprint.h"


#define FOVY      60.0f
#define NEAR_CLIP 0.001f
#define FAR_CLIP  10.0f

#define MOUSE_SCALE (1.0f/128.0f)

static const QString shaders_dir = ":/shaders/ShaderLib/shaders/";
static const QString vertex_path[3] = {
	"axes.vert", "axes_box.vert", "spheres.vert"
};
static const QString fragment_path[3] = {
	"axes.frag", "axes_box.frag", "spheres.frag"
};

struct ColoredVertex
{
	QVector3D position;
	QVector4D color;
};

#define AXES_COMPONENTS 6
#define AXES_BOX_COMPONENTS 18
#define OFFSET(o) (const void*)(o)

static const ColoredVertex axes[AXES_COMPONENTS] = {
	// X - Red
	{QVector3D(0.0f, 0.0f, 0.0f), QVector4D(1.0f, 0.0f, 0.0f, 1.0f)},
	{QVector3D(1.0f, 0.0f, 0.0f), QVector4D(0.0f, 0.0f, 0.0f, 1.0f)},
	// Y - Green
	{QVector3D(0.0f, 0.0f, 0.0f), QVector4D(0.0f, 1.0f, 0.0f, 1.0f)},
	{QVector3D(0.0f, 1.0f, 0.0f), QVector4D(0.0f, 0.0f, 0.0f, 1.0f)},
	// Z - Blue
	{QVector3D(0.0f, 0.0f, 0.0f), QVector4D(0.0f, 0.0f, 1.0f, 1.0f)},
	{QVector3D(0.0f, 0.0f, 1.0f), QVector4D(0.0f, 0.0f, 0.0f, 1.0f)},
};

static const QVector4D axes_box_color = QVector4D(0.0f, 0.0f, 0.0f, 1.0f);
static const QVector3D axes_box[AXES_BOX_COMPONENTS] = {
	QVector3D(1.0f, 1.0f, 1.0f), QVector3D(0.0f, 1.0f, 1.0f),
	QVector3D(1.0f, 1.0f, 1.0f), QVector3D(1.0f, 0.0f, 1.0f),
	QVector3D(1.0f, 1.0f, 1.0f), QVector3D(1.0f, 1.0f, 0.0f),
	QVector3D(1.0f, 0.0f, 0.0f), QVector3D(1.0f, 1.0f, 0.0f),
	QVector3D(1.0f, 0.0f, 0.0f), QVector3D(1.0f, 0.0f, 1.0f),
	QVector3D(0.0f, 1.0f, 0.0f), QVector3D(1.0f, 1.0f, 0.0f),
	QVector3D(0.0f, 1.0f, 0.0f), QVector3D(0.0f, 1.0f, 1.0f),
	QVector3D(0.0f, 0.0f, 1.0f), QVector3D(0.0f, 1.0f, 1.0f),
	QVector3D(0.0f, 0.0f, 1.0f), QVector3D(1.0f, 0.0f, 1.0f)
};


GLScatter::GLScatter(QWidget *parent) :
		QGLWidget(parent)
{
	rotation = QQuaternion();
	translation = QVector3D();
	camDist = 5.0f;
	clearColor = Qt::white;

	points = 0;
	elements = 0;
	psize = 0.01;
}

GLScatter::~GLScatter()
{
	glDeleteBuffers(vboLast, vbos);
}

void GLScatter::resetView(void)
{
	rotation = QQuaternion();
	translation = QVector3D();
	camDist = 5.0f;
	updateGL();
}

void GLScatter::setClearColor(QColor color)
{
	clearColor = color;
	qglClearColor(clearColor);
	updateGL();
}

void GLScatter::setData(float *positions, float *colors, int p, int e)
{
	elements = e;
	points = p;
	size_t elem_size = elements * points * sizeof(float);
	glBindBuffer(GL_ARRAY_BUFFER, vbos[vboDataVertex]);
	glBufferData(GL_ARRAY_BUFFER, elem_size, positions, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vbos[vboDataColor]);
	glBufferData(GL_ARRAY_BUFFER, elem_size, colors, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLScatter::initializeGL()
{
	initializeGLFunctions();
	glGenBuffers(vboLast, vbos);
	initShaders();

	qglClearColor(clearColor);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

	// Bind buffers
	glBindBuffer(GL_ARRAY_BUFFER, vbos[vboAxes]);
	glBufferData(GL_ARRAY_BUFFER, AXES_COMPONENTS * sizeof(ColoredVertex),
				 axes, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vbos[vboAxesBox]);
	glBufferData(GL_ARRAY_BUFFER, AXES_BOX_COMPONENTS * sizeof(QVector3D),
				 axes_box, GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLScatter::initShaders()
{
	bool status = true;
	// Override system locale until shaders are compiled
	setlocale(LC_NUMERIC, "C");

	for (int i = 0; i < progLast; ++i) {
		QString vp = shaders_dir + vertex_path[i];
		QString fp = shaders_dir + fragment_path[i];
		status = program[i].addShaderFromSourceFile(QGLShader::Vertex, vp);
		if (status)
			status = program[i].addShaderFromSourceFile(QGLShader::Fragment, fp);
		if (status)
			status = program[i].link();
		if (!status) {
			dbgPrint(DBGLVL_ERROR, "Failed to initialize glScatter shaders:\n%s, %s:\n%s",
					 vp.toStdString().c_str(), fp.toStdString().c_str(),
					 program[i].log().toStdString().c_str());
			exit(1);
		}
	}

	// Restore system locale
	setlocale(LC_ALL, "");

	// Bind global uniforms
	program[progAxesBox].bind();
	program[progAxesBox].setUniformValue("color", axes_box_color);
	program[progAxesBox].release();
}

void GLScatter::paintGL()
{
	QMatrix4x4 matrix;
	int w = width();
	int h = height();
	float cDist = 1.0f / tan(FOVY / 360.0f * M_PI);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	matrix.translate(0.0, 0.0, -cDist);
	matrix.translate(0.0, 0.0, -camDist);
	matrix.translate(translation);

	//glRotatef(rotAngle * 180.0 / M_PI, rotAxis.x, rotAxis.y, rotAxis.z);
	matrix.rotate(rotation);
	matrix.translate(-0.5f, -0.5f, -0.5f);
	QMatrix4x4 mvp = projection * matrix;

	drawAxes(mvp);

	QGLShaderProgram *prog = &program[progData];
	prog->bind();
	prog->setUniformValue("vpParams", 2.0 / w, 2.0 / h, 0.5 * w, 0.5 * h);
	prog->setUniformValue("psize", psize);
	drawArray(prog, mvp, GL_POINTS, points,
			  vbos[vboDataVertex], sizeof(float), elements, 0,
			  vbos[vboDataColor], sizeof(float), elements, 0);
	prog->release();
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void GLScatter::resizeGL(int width, int height)
{
	qreal aspect = qreal(width) / qreal(height ? height : 1);
	glViewport(0, 0, width, height);
	projection.setToIdentity();
	projection.perspective(FOVY, aspect, NEAR_CLIP, FAR_CLIP);
}

void GLScatter::drawAxes(QMatrix4x4 &mvp)
{
	QGLShaderProgram *pcurrent;

	// Draw axes
	pcurrent = &program[progAxes];
	pcurrent->bind();
	drawArray(pcurrent, mvp, GL_LINES, AXES_COMPONENTS,
			  vbos[vboAxes], sizeof(ColoredVertex), 3, 0,
			  vbos[vboAxes], sizeof(QVector4D), 4, sizeof(QVector3D));

	// Draw axes box
	pcurrent = &program[progAxesBox];
	pcurrent->bind();
	drawArray(pcurrent, mvp, GL_LINES, AXES_BOX_COMPONENTS,
			  vbos[vboAxesBox], sizeof(QVector3D), 3, 0);

	pcurrent->release();
}

void GLScatter::drawArray(QGLShaderProgram *prog, QMatrix4x4 &mvp, GLenum type, int components,
					  GLuint vbo_pos, size_t size_pos, int count_pos, int offset_pos,
					  GLuint vbo_color, size_t size_color, int count_color, int offset_color )
{
	prog->setUniformValue("mvp", mvp);

	int vloc = prog->attributeLocation("position");
	prog->enableAttributeArray(vloc);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_pos);
	glVertexAttribPointer(vloc, count_pos, GL_FLOAT, GL_FALSE, size_pos, OFFSET(offset_pos));

	if (count_color) {
		int cloc = prog->attributeLocation("color");
		prog->enableAttributeArray(cloc);

		if (vbo_color)
			glBindBuffer(GL_ARRAY_BUFFER, vbo_color);

		glVertexAttribPointer(cloc, count_color, GL_FLOAT, GL_FALSE, size_color,
							  OFFSET(offset_color));
	}

	glDrawArrays(type, 0, components);
}

void GLScatter::mousePressEvent(QMouseEvent *event)
{
	lastMousePos = event->pos();
}

void GLScatter::mouseMoveEvent(QMouseEvent *event)
{
	QVector3D view(0, 0, -1);
	QVector3D mouse(
		MOUSE_SCALE * (event->x() - lastMousePos.x()),
		-MOUSE_SCALE * (event->y() - lastMousePos.y()),
		0.0
	);

	QVector3D rotAxis = QVector3D::crossProduct(mouse, view);
	rotAxis.normalize();

	int buttons = event->buttons();
	if (buttons & Qt::LeftButton) {
		QQuaternion rot = QQuaternion::fromAxisAndAngle(rotAxis,
									pow(mouse.x(), 2) + pow(mouse.y(), 2));
		rotation = rot * rotation;
		rotation.normalize();
	} else if (buttons & Qt::RightButton) {
		camDist += 0.5f * (mouse.x() + mouse.y());
	} else if (buttons & Qt::MidButton) {
		translation += mouse;
	}

	lastMousePos = event->pos();
	updateGL();
}

