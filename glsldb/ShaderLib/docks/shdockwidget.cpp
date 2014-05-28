#include "shdockwidget.h"

ShVarModel* ShDockWidget::model = NULL;

ShDockWidget::ShDockWidget(QWidget *parent) :
	QDockWidget(parent)
{
	model = NULL;
}

void ShDockWidget::setModel(ShVarModel *_model)
{
	model = _model;
}
