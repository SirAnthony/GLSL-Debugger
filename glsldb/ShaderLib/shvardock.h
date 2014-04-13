#ifndef SHVARDOCK_H
#define SHVARDOCK_H

#include <QDockWidget>
#include <QSortFilterProxyModel>
#include "shvarmodel.h"

namespace Ui {
	class ShVarDock;
}

class ShVarDock : public QDockWidget
{
	Q_OBJECT
public:
	explicit ShVarDock(QWidget *parent = 0);
	~ShVarDock();

	typedef enum {
		trAll = 0,
		trBuiltin,
		trScope,
		trWatchList,
		trUniform,
		trLast
	} tabRole;
	
signals:
	
public slots:

private:
	template<class T>
	inline QWidget* newTab() { return new T(this->model); }

	ShVarModel* model;
	Ui::ShVarDock *ui;
	
};

#endif // SHVARDOCK_H
