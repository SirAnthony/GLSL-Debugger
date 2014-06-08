#ifndef SHDOCKWIDGET_H
#define SHDOCKWIDGET_H

#include <QDockWidget>
#include "models/shvarmodel.h"

class ShDataManager;

class ShDockWidget : public QDockWidget
{
	Q_OBJECT
public:
	explicit ShDockWidget(QWidget *parent = 0);
	void setModel(ShVarModel*);
	virtual void registerDock(ShDataManager*) = 0;

	typedef enum {
		trAll = 0,
		trBuiltin,
		trScope,
		trWatchList,
		trUniform,
		trLast
	} tabRole;

signals:
	void updateWindows(bool force);

public slots:
	virtual void cleanDock(ShaderMode) = 0;

protected:
	ShVarModel* model;
};

#endif // SHDOCKWIDGET_H
