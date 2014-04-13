#ifndef SHVARDOCK_H
#define SHVARDOCK_H

#include "shdockwidget.h"

namespace Ui {
	class ShVarDock;
}

class ShVarDock : public ShDockWidget
{
	Q_OBJECT
public:
	explicit ShVarDock(QWidget *parent = 0);
	~ShVarDock();

private:
	template<class T>
	inline QWidget* newTab() { return new T(this->model); }

	Ui::ShVarDock *ui;
	
};

#endif // SHVARDOCK_H
