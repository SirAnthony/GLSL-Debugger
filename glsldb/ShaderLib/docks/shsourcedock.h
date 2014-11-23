#ifndef SHSOURCEDOCK_H
#define SHSOURCEDOCK_H

#include "shdockwidget.h"
#include "dialogs/fragmenttest.h"

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

	virtual void registerDock(ShDataManager *);

signals:
	void stepShader(int);
	void resetShader();
	void executeShader(ShaderMode);

public slots:
	void updateGui(int, bool, bool);
	void setGuiUpdates(bool);
	void getShaders(char **shaders, int count);
	void setShaders(const char **shaders, int count);
	void executeShader();
	void stepInto();
	void stepOver();
	virtual void cleanDock(ShaderMode);
	void updateStepControls(bool);
	void updateControls();
	void updateHighlight(ShaderMode, DbgRsRange *);
	void getOptions(FragmentTestOptions *);
	void getCurrentIndex(int&);

protected slots:
	void currentChanged(int);
	void updateCurrent(int);
	void showOptions();

private:
	Ui::ShSourceDock *ui;
	FragmentTestOptions options;
	QTextEdit *editWidgets[smCount];
	QString shaderText[smCount];
};

#endif // SHSOURCEDOCK_H
