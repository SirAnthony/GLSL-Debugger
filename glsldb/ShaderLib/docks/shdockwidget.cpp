#include "shdockwidget.h"

ShDockWidget::ShDockWidget(QWidget *parent) :
	QDockWidget(parent)
{
	model = NULL;
}

void ShDockWidget::setModel(ShVarModel *_model)
{
	model = _model;
}
