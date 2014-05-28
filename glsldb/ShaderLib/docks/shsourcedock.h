#ifndef SHSOURCEDOCK_H
#define SHSOURCEDOCK_H

#include "shdockwidget.h"
#include "ShaderLang.h"

namespace Ui {
	class ShSourceDock;
}

class ShSourceDock : public ShDockWidget
{
	Q_OBJECT
public:
	explicit ShSourceDock(QWidget *parent = 0);
	~ShSourceDock();

	void setShaders(const char *shaders[3]);
	void getSource(const char *shaders[3]);	

signals:
	
public slots:
	virtual void cleanDock(EShLanguage);
	void updateControls(bool);
	void updateHighlight(EShLanguage, DbgRsRange &);

protected:
	void getEdit(EShLanguage);

private:
	QString shaderText[3];
	Ui::ShSourceDock *ui;
};

#endif // SHSOURCEDOCK_H
