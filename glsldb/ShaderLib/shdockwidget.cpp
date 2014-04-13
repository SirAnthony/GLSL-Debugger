#include "shdockwidget.h"

ShVarModel* ShDockWidget::model = NULL;

ShDockWidget::ShDockWidget(QWidget *parent) :
	QDockWidget(parent)
{
	if (!this->model)
		this->model = new ShVarModel();
}
