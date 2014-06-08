
#include "shvardock.h"
#include "ui_shvardock.h"
#include "shproxytreeview.h"
#include "shdatamanager.h"
#include "watch/shwindowmanager.h"

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

void ShVarDock::registerDock(ShDataManager *manager)
{
	setModel(manager->getModel());
	connect(manager, SIGNAL(cleanDocks(ShaderMode)), this, SLOT(cleanDock(ShaderMode)));
	connect(this, SIGNAL(updateWindows(bool)),
			manager->getWindows(), SLOT(updateWindows(bool)));
}
