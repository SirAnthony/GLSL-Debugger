#include "shdatamanager.h"
#include "shsourcedock.h"
#include "ui_shsourcedock.h"
#include "glslSyntaxHighlighter.h"
#include "utils/dbgprint.h"

#include <QTextDocument>
#include <QTextCharFormat>
#include <QTextEdit>


ShSourceDock::ShSourceDock(QWidget *parent) :
	ShDockWidget(parent)
{
	ui->setupUi(this);
	ShDataManager::get()->registerDock(this, ShDataManager::dmDTSource);
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
		shaderText[s] = QString(shaders && shaders[s] ? shaders[s] : "");
		QTextDocument *doc = new QTextDocument(shaderText[s], edits[s]);
		edits[s]->setDocument(doc);
		if (!shaderText[s].isEmpty()) {
			/* the document becomes owner of the highlighter, so it get's freed */
			new GlslSyntaxHighlighter(doc);
			edits[s]->setDocument(doc);
			edits[s]->setTabStopWidth(30);
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

void ShSourceDock::cleanDock(EShLanguage type)
{
	/* remove debug markers from code display */
	QTextDocument *document = NULL;
	QTextEdit *edit = NULL;
	switch (type) {
	case EShLangVertex:
		edit = ui->teVertex;
		break;
	case EShLangGeometry:
		edit = ui->teGeometry;
		break;
	case EShLangFragment:
		edit = ui->teFragment;
		break;
	default:
		dbgPrint(DBGLVL_INTERNAL_WARNING, "Wrong type passed to cleanDock.");
		break;
	}

	if (edit)
		document = edit->document();

	if (document && edit) {
		QTextCharFormat highlight;
		QTextCursor cursor(document);

		cursor.setPosition(0, QTextCursor::MoveAnchor);
		cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor, 1);
		highlight.setBackground(Qt::white);
		cursor.mergeCharFormat(highlight);
	}
}

