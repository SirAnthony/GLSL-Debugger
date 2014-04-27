#ifndef SHSOURCEDOCK_H
#define SHSOURCEDOCK_H

#include <QDockWidget>

namespace Ui {
	class ShSourceDock;
}

class ShSourceDock : public QDockWidget
{
	Q_OBJECT
public:
	explicit ShSourceDock(QWidget *parent = 0);
	~ShSourceDock();

	void setShaders(const char* shaders[3]);
	void getSource(const char* shaders[3]);
	
signals:
	
public slots:

private:
	QString shaderText[3];
	Ui::ShSourceDock *ui;
};

#endif // SHSOURCEDOCK_H
