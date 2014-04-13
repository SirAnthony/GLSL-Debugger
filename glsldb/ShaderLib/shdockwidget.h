#ifndef SHDOCKWIDGET_H
#define SHDOCKWIDGET_H

#include <QDockWidget>
#include "shvarmodel.h"


class ShDockWidget : public QDockWidget
{
	Q_OBJECT
public:
	explicit ShDockWidget(QWidget *parent = 0);

	typedef enum {
		trAll = 0,
		trBuiltin,
		trScope,
		trWatchList,
		trUniform,
		trLast
	} tabRole;

private:
	static ShVarModel* model;
};

#endif // SHDOCKWIDGET_H
