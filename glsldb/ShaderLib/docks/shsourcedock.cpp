#include "shdatamanager.h"
#include "shsourcedock.h"
#include "ui_shsourcedock.h"
#include "glslSyntaxHighlighter.h"
#include "utils/dbgprint.h"

#include <QTextDocument>
#include <QTextCharFormat>
#include <QTextEdit>


ShSourceDock::ShSourceDock(QWidget *parent) :
	ShDockWidget(parent), editWidgets {
		ui->teVertex, ui->teGeometry, ui->teFragment }
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
	for (int s = 0; s < smCount; ++s) {
		shaderText[s] = QString(shaders && shaders[s] ? shaders[s] : "");
		QTextDocument *doc = new QTextDocument(shaderText[s], editWidgets[s]);
		editWidgets[s]->setDocument(doc);
		if (!shaderText[s].isEmpty()) {
			/* the document becomes owner of the highlighter, so it get's freed */
			new GlslSyntaxHighlighter(doc);
			editWidgets[s]->setDocument(doc);
			editWidgets[s]->setTabStopWidth(30);
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

/*
 * Remove debug markers from code display
 */
void ShSourceDock::cleanDock(ShaderMode type)
{
	if (type < 0 || type > smCount)
		return;

	QTextDocument *document = editWidgets[type]->document();
	if (!document)
		return;

	QTextCharFormat highlight;
	QTextCursor cursor(document);

	cursor.setPosition(0, QTextCursor::MoveAnchor);
	cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor, 1);
	highlight.setBackground(Qt::white);
	cursor.mergeCharFormat(highlight);
}

void ShSourceDock::updateControls(bool enabled)
{
	ui->tbStep->setEnabled(enabled);
	ui->tbJumpOver->setEnabled(enabled);
}

/*
 * Update highlight of source views
 */
void ShSourceDock::updateHighlight(ShaderMode type, DbgRsRange &range)
{
	if (type < 0 || type > smCount)
		return;

	QTextEdit *edit = editWidgets[type];
	QTextDocument *document = edit->document();
	if (!document)
		return;

	/* Mark actual debug position */
	QTextCharFormat highlight;
	QTextCursor cursor(document);
	cursor.setPosition(0, QTextCursor::MoveAnchor);
	cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor, 1);
	highlight.setBackground(Qt::white);
	cursor.mergeCharFormat(highlight);

	/* Highlight the actual statement */
	cursor.setPosition(0, QTextCursor::MoveAnchor);
	cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, range.left.line - 1);
	cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, range.left.colum - 1);
	cursor.setPosition(0, QTextCursor::KeepAnchor);
	cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, range.right.line - 1);
	cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, range.right.colum);
	highlight.setBackground(Qt::yellow);
	cursor.mergeCharFormat(highlight);

	/* Ensure the highlighted line is visible */
	QTextCursor cursorVisible = edit->textCursor();
	cursorVisible.setPosition(0, QTextCursor::MoveAnchor);
	cursorVisible.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor,
							   std::max(range.left.line - 3, 0));
	edit->setTextCursor(cursorVisible);
	edit->ensureCursorVisible();
	cursorVisible.setPosition(0, QTextCursor::KeepAnchor);
	cursorVisible.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor,
							   range.right.line + 1);
	edit->setTextCursor(cursorVisible);
	edit->ensureCursorVisible();

	/* Unselect visible cursor */
	QTextCursor cursorSet = edit->textCursor();
	cursorSet.setPosition(0, QTextCursor::MoveAnchor);
	cursorSet.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, range.left.line - 1);
	edit->setTextCursor(cursorSet);
	qApp->processEvents();
}
