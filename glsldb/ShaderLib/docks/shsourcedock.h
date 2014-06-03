#ifndef SHSOURCEDOCK_H
#define SHSOURCEDOCK_H

#include "shdockwidget.h"
#include "dialogs/fragmenttest.h"
#include "ShaderLang.h"

class QTextEdit;

namespace Ui {
	class ShSourceDock;
}

class ShSourceDock : public ShDockWidget
{
	Q_OBJECT
public:
	explicit ShSourceDock(QWidget *parent = 0);
	~ShSourceDock();	

	void getSource(const char *shaders[smCount]);

signals:
	void stepShader(int);
	void resetShader();
	void executeShader(ShaderMode);
	
public slots:
	void setShaders(const char *shaders[smCount]);
	void executeShader();
	void stepInto();
	void stepOver();
	virtual void cleanDock(ShaderMode);
	void updateControls(bool);
	void updateHighlight(ShaderMode, DbgRsRange &);
	void getOptions(FragmentTestOptions *);

protected slots:
	void currentChanged(int);
	void showOptions();

private:
	FragmentTestOptions options;
	QTextEdit *editWidgets[smCount];
	QString shaderText[smCount];
	Ui::ShSourceDock *ui;
};

#endif // SHSOURCEDOCK_H
