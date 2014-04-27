#include "shsourcedock.h"
#include "ui_shsourcedock.h"

ShSourceDock::ShSourceDock(QWidget *parent) :
	QDockWidget(parent)
{
	ui->setupUi(this);
}

ShSourceDock::~ShSourceDock()
{
	delete ui;
}

void ShSourceDock::setShaders(const char *shaders[])
{
	QTextEdit *edits[3] = {
		ui->teVertex, ui->teGeometry, ui->teFragment
	};

	for (int s = 0; s < 3; ++s) {
		shaderText[s] = QString(shaders && shaders[i] ? shaders[i] : "");
		QTextDocument *doc = new QTextDocument(shaderText[s], edits[i]);
		edits[i]->setDocument(doc);
		if (shaderText[s]) {
			/* the document becomes owner of the highlighter, so it get's freed */
			GlslSyntaxHighlighter *highlighter = new GlslSyntaxHighlighter(doc);
			edits[i]->setDocument(doc);
			edits[i]->setTabStopWidth(30);
		}
	}
}

void ShSourceDock::getSource(const char *shaders[])
{
	if (!shaders)
		return;
	for (int s = 0; s < 3; ++s) {
		shaders[s] = shaderText[s].toStdString().c_str();
	}
}

