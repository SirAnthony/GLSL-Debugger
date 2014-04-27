
#include "shvardock.h"
#include "ui_shvardock.h"
#include "shproxytreeview.h"

ShVarDock::ShVarDock(QWidget *parent) :
	ShDockWidget(parent), ui(new Ui::ShVarDock)
{
	ui->setupUi(this);

	ui->tTabWidget->addTab(newTab<ShProxyTreeView>(), "All");
	ui->tTabWidget->addTab(newTab<ShBuiltInTreeView>(), "Builtin");
	ui->tTabWidget->addTab(newTab<ShScopeTreeView>(), "Scope");
	ui->tTabWidget->addTab(newTab<ShUniformTreeView>(), "Uniform");

	// In another dock
	//ui->tTabWidget->addTab(newTab<ShWatchedTreeView>(), "Watch list");
}

ShVarDock::~ShVarDock()
{
	delete ui;
}
