#include "shdatamanager.h"
#include "shsourcedock.h"
#include "ui_shsourcedock.h"
#include "glslSyntaxHighlighter.h"
#include "watch/shwindowmanager.h"
#include "utils/dbgprint.h"
#include "ShaderLang.h"

#include <QTextDocument>
#include <QTextCharFormat>
#include <QTextEdit>
#include <initializer_list>


ShSourceDock::ShSourceDock(QWidget *parent) :
	ShDockWidget(parent), ui(new Ui::ShSourceDock)
{
	ui->setupUi(this);
	updateGui(-1, false, false);

	connect(ui->twShader, SIGNAL(currentChanged(int)), this, SLOT(currentChanged(int)));
	auto widgets = std::initializer_list<QTextEdit *>({
		ui->teVertex, ui->teGeometry, ui->teFragment });
	std::copy(widgets.begin(), widgets.end(), editWidgets);


	connect(ui->tbReset, SIGNAL(clicked()), this, SIGNAL(resetShader()));

	for (int i = 0; i < smCount; ++i)
		editWidgets[i]->setTabStopWidth(30);
}

ShSourceDock::~ShSourceDock()
{
	delete ui;
}

void ShSourceDock::registerDock(ShDataManager *manager)
{
	setModel(manager->getModel());
	connect(manager, SIGNAL(cleanDocks(ShaderMode)), this, SLOT(cleanDock(ShaderMode)));
	connect(this, SIGNAL(updateWindows(bool)),
			manager->getWindows(), SLOT(updateWindows(bool)));

	connect(manager, SIGNAL(setGuiUpdates(bool)), this, SLOT(setGuiUpdates(bool)));
	connect(manager, SIGNAL(updateStepControls(bool)),
			this, SLOT(updateStepControls(bool)));
	connect(manager, SIGNAL(updateControls(int,bool,bool)),
			this, SLOT(updateGui(int,bool,bool)));
	connect(manager, SIGNAL(updateSourceHighlight(ShaderMode,DbgRsRange*)),
			this, SLOT(updateHighlight(ShaderMode,DbgRsRange*)));
	connect(manager, SIGNAL(getShaders(const char**)),
			this, SLOT(getShaders(const char**)));
	connect(manager, SIGNAL(setShaders(const char**)),
			this, SLOT(setShaders(const char**)));
	connect(manager, SIGNAL(getCurrentIndex(int&)), this, SLOT(getCurrentIndex(int&)));
	connect(this, SIGNAL(stepShader(int)), manager, SLOT(step(int)));
	connect(this, SIGNAL(resetShader()), manager, SLOT(reset()));
	connect(this, SIGNAL(executeShader(ShaderMode)), manager, SLOT(execute(ShaderMode)));
}

void ShSourceDock::getShaders(const char **shaders)
{
	if (!shaders)
		return;
	for (int s = 0; s < 3; ++s)
		shaders[s] = shaderText[s].toStdString().c_str();
}

void ShSourceDock::setShaders(const char **shaders)
{
	for (int s = 0; s < smCount; ++s) {
		shaderText[s] = QString((shaders && shaders[s]) ? shaders[s] : "");
		QTextDocument *doc = new QTextDocument(shaderText[s], editWidgets[s]);
		/* the document becomes owner of the highlighter, so it get's freed */
		if (!shaderText[s].isEmpty())
			new GlslSyntaxHighlighter(doc);
		editWidgets[s]->setDocument(doc);
		editWidgets[s]->setTabStopWidth(30);
	}
}

void ShSourceDock::executeShader()
{
	ShaderMode mode = static_cast<ShaderMode>(ui->twShader->currentIndex());
	emit executeShader(mode);
}

void ShSourceDock::updateGui(int active_tab, bool restart, bool debuggable)
{
	static QString execute_icon = ":/icons/icons/shader-execute_32.png";
	static QString devil_icon = ":/icons/icons/face-devil-grin_32.png";
	bool enabled = active_tab >= 0;
	QString icon_string = enabled ? execute_icon : devil_icon;

	ui->tbReset->setEnabled(enabled);
	ui->tbStep->setEnabled(enabled);
	ui->tbJumpOver->setEnabled(enabled);
	ui->tbExecute->setEnabled(enabled);
	ui->tbOptions->setEnabled(!restart && !debuggable);

	if (enabled || (!restart && debuggable))
		updateControls();

	if (!restart)
		ui->tbExecute->setIcon(QIcon(icon_string));

	for (int i = 0; i < smCount; ++i) {
		QIcon icon;
		if (i == active_tab)
			icon.addFile(execute_icon);
		ui->twShader->setTabIcon(i, icon);
	}
}

void ShSourceDock::setGuiUpdates(bool enabled)
{
	ui->teFragment->setUpdatesEnabled(enabled);
	ui->teGeometry->setUpdatesEnabled(enabled);
	ui->teVertex->setUpdatesEnabled(enabled);
}

void ShSourceDock::stepInto()
{
	emit stepShader(DBG_BH_JUMP_INTO);
}

void ShSourceDock::stepOver()
{
	// TODO: else is not step_over action
	emit stepShader(DBG_BH_FOLLOW_ELSE);
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

void ShSourceDock::updateStepControls(bool enabled)
{
	ui->tbStep->setEnabled(enabled);
	ui->tbJumpOver->setEnabled(enabled);
}

void ShSourceDock::updateControls()
{
	int index = ui->twShader->currentIndex();
	if (index >= 0)
		updateCurrent(index);
}

void ShSourceDock::updateCurrent(int index)
{
	ShBuiltInResource *resource = ShDataManager::get()->getResource();
	bool status = shaderText[index].length();
	if (index == smVertex)
		status = status && resource->transformFeedbackSupported;
	else if (index == smGeometry)
		status = status && resource->geoShaderSupported &&
				resource->transformFeedbackSupported;
	else if (index == smFragment)
		status = status && resource->framebufferObjectsSupported;

	ui->tbExecute->setEnabled(status);
	ui->tbOptions->setEnabled(index == smFragment && status);
}

/*
 * Update highlight of source views
 */
void ShSourceDock::updateHighlight(ShaderMode type, DbgRsRange *range)
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
	cursor.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, range->left.line - 1);
	cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, range->left.colum - 1);
	cursor.setPosition(0, QTextCursor::KeepAnchor);
	cursor.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor, range->right.line - 1);
	cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, range->right.colum);
	highlight.setBackground(Qt::yellow);
	cursor.mergeCharFormat(highlight);

	/* Ensure the highlighted line is visible */
	QTextCursor cursorVisible = edit->textCursor();
	cursorVisible.setPosition(0, QTextCursor::MoveAnchor);
	cursorVisible.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor,
							   std::max(range->left.line - 3, 0));
	edit->setTextCursor(cursorVisible);
	edit->ensureCursorVisible();
	cursorVisible.setPosition(0, QTextCursor::KeepAnchor);
	cursorVisible.movePosition(QTextCursor::Down, QTextCursor::KeepAnchor,
							   range->right.line + 1);
	edit->setTextCursor(cursorVisible);
	edit->ensureCursorVisible();

	/* Unselect visible cursor */
	QTextCursor cursorSet = edit->textCursor();
	cursorSet.setPosition(0, QTextCursor::MoveAnchor);
	cursorSet.movePosition(QTextCursor::Down, QTextCursor::MoveAnchor, range->left.line - 1);
	edit->setTextCursor(cursorSet);
	qApp->processEvents();
}

void ShSourceDock::getOptions(FragmentTestOptions *opts)
{
	if (!opts)
		return;
	memcpy(opts, &options, sizeof(FragmentTestOptions));
}

void ShSourceDock::getCurrentIndex(int &index)
{
	index = ui->twShader->currentIndex();
}

void ShSourceDock::currentChanged(int index)
{
	ShDataManager *manager = ShDataManager::get();
	ShaderMode mode = manager->getMode();

	if (mode > 0) {
		bool status = mode == index;
		ui->tbExecute->setEnabled(status);
		ui->tbReset->setEnabled(status);
		ui->tbJumpOver->setEnabled(status);
		ui->tbStep->setEnabled(status);
		if (!status)
			ui->tbOptions->setEnabled(false);
	} else if (manager->codeReady()) {
		updateCurrent(index);
	} else {
		ui->tbExecute->setEnabled(false);
	}
}


void ShSourceDock::showOptions()
{
	FragmentTestDialog dialog(options);
	dialog.exec();
}
