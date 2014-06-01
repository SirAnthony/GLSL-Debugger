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

	void setShaders(const char *shaders[smCount]);	
	void getSource(const char *shaders[smCount]);

signals:
	
public slots:
	virtual void cleanDock(ShaderMode);
	void updateControls(bool);
	void updateHighlight(ShaderMode, DbgRsRange &);
	void getOptions(FragmentTestOptions *);

protected slots:
	void showOptions();

private:
	FragmentTestOptions options;
	QTextEdit *editWidgets[smCount];
	QString shaderText[smCount];
	Ui::ShSourceDock *ui;
};

#endif // SHSOURCEDOCK_H
