/******************************************************************************

Copyright (C) 2006-2009 Institute for Visualization and Interactive Systems
(VIS), Universität Stuttgart.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice, this
    list of conditions and the following disclaimer in the documentation and/or
    other materials provided with the distribution.

  * Neither the name of the name of VIS, Universität Stuttgart nor the names
    of its contributors may be used to endorse or promote products derived from
    this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

#include <QDesktopServices>
#include <QTextDocument>
#include <QMessageBox>
#include <QHeaderView>
#include <QGridLayout>
#include <QFileDialog>
#include <QSettings>
#include <QWorkspace>
#include <QTabWidget>
#include <QTabBar>
#include <QColor>
#include <QUrl>
#include <QCloseEvent>
#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif /* _WIN32 */

#include "mainWindow.qt.h"
#include "editCallDialog.qt.h"
//#include "dbgShaderView.qt.h"
#include "attachToProcessDialog.qt.h"
#include "aboutBox.qt.h"
#include "ShaderLib/shdatamanager.h"
#include "ShaderLib/data/pixelBox.h"

#include "runLevel.h"
#include "debuglib.h"
#include "utils/dbgprint.h"
#include "utils/notify.h"

#define MAX(a,b) ( a < b ? b : a )
#define MIN(a,b) ( a > b ? b : a )

#ifdef _WIN32
#define REGISTRY_KEY "Software\\VIS\\glslDevil"
#endif /* _WIN32 */

extern "C" GLFunctionList glFunctions[];

MainWindow::MainWindow(char *pname, const QStringList& args) :
		dbgProgArgs(args)
{
	pc = new ProgramControl(pname);
	shaderManager = ShDataManager::create(this, pc);
	connect(shaderManager, SIGNAL(saveQueries(int&)), this, SLOT(saveQueries(int&)));
	connect(shaderManager, SIGNAL(setRunLevel(int)), this, SLOT(setRunLevel(int)));
	connect(shaderManager, SIGNAL(setCurrentDebuggable(int&,bool)),
			this, SLOT(setRunLevelDebuggable(int&,bool)));
	connect(shaderManager, SIGNAL(killProgram(int)), this, SLOT(killProgram(int)));
	connect(shaderManager, SIGNAL(setErrorStatus(int)), this, SLOT(setErrorStatus(int)));
	connect(shaderManager, SIGNAL(recordDrawCall(int&)),
			this, SLOT(recordDrawCall(int&)));
	connect(this, SIGNAL(updateShaders(int&)), shaderManager, SLOT(updateShaders(int&)));
	connect(this, SIGNAL(cleanShader()), shaderManager, SLOT(cleanShader()));
	connect(this, SIGNAL(removeShaders()), shaderManager, SLOT(removeShaders()));

	/*** Setup GUI ****/
	setupUi(this);

	connect(tbToggleGuiUpdate, SIGNAL(toggled(bool)), shaderManager,
			SIGNAL(setGuiUpdates(bool)));

	// Register docks
	dwShaderSource->registerDock(shaderManager);
	dwWatch->registerDock(shaderManager);
	dwShVar->registerDock(shaderManager);

	/* Functionality seems to be obsolete now */
	tbToggleGuiUpdate->setVisible(false);

	setAttribute(Qt::WA_QuitOnClose, true);

	/*   Status bar    */
	statusbar->addPermanentWidget(fSBError);
	statusbar->addPermanentWidget(fSBMouseInfo);

	/*   Buffer View    */
	QGridLayout *gridLayout;
	gridLayout = new QGridLayout(fContent);
	gridLayout->setSpacing(0);
	gridLayout->setMargin(0);
	sBVArea = new QScrollArea();
	sBVArea->setBackgroundRole(QPalette::Dark);
	gridLayout->addWidget(sBVArea);
	lBVLabel = new QLabel();
	sBVArea->setWidget(lBVLabel);

	/* GLTrace Settings */
	m_pGlTraceFilterModel = new GlTraceFilterModel(glFunctions, this);
	m_pgtDialog = new GlTraceSettingsDialog(m_pGlTraceFilterModel, this);

	/*   GLTrace View   */
	GlTraceListFilterModel *lvGlTraceFilter = new GlTraceListFilterModel(
			m_pGlTraceFilterModel, this);
	m_pGlTraceModel = new GlTraceListModel(MAX_GLTRACE_ENTRIES,
			m_pGlTraceFilterModel, this);
	lvGlTraceFilter->setSourceModel(m_pGlTraceModel);
	lvGlTrace->setModel(lvGlTraceFilter);

	/* Action Group for watch window controls */
	agWatchControl = new QActionGroup(this);
	agWatchControl->addAction(aZoom);
	agWatchControl->addAction(aSelectPixel);
	agWatchControl->addAction(aMinMaxLens);
	agWatchControl->setEnabled(false);

	m_pCurrentCall = NULL;

	if (dbgProgArgs.size())
		setRunLevel(RL_SETUP);
	else
		setRunLevel(RL_INIT);

	m_bInDLCompilation = false;

	m_pGlCallSt = new GlCallStatistics(tvGlCalls);
	m_pGlExtSt = new GlCallStatistics(tvGlExt);
	m_pGlCallPfst = new GlCallStatistics(tvGlCallsPf);
	m_pGlExtPfst = new GlCallStatistics(tvGlExtPf);
	m_pGlxCallSt = new GlCallStatistics(tvGlxCalls);
	m_pGlxExtSt = new GlCallStatistics(tvGlxExt);
	m_pGlxCallPfst = new GlCallStatistics(tvGlxCallsPf);
	m_pGlxExtPfst = new GlCallStatistics(tvGlxExtPf);
	m_pWglCallSt = new GlCallStatistics(tvWglCalls);
	m_pWglExtSt = new GlCallStatistics(tvWglExt);
	m_pWglCallPfst = new GlCallStatistics(tvWglCallsPf);
	m_pWglExtPfst = new GlCallStatistics(tvWglExtPf);

	while (twGlStatistics->count() > 0)
		twGlStatistics->removeTab(0);

	twGlStatistics->insertTab(0, taGlCalls, QString("GL Calls"));
	twGlStatistics->insertTab(1, taGlExt, QString("GL Extensions"));

#ifdef _WIN32
	// TODO: Only Windows can attach at the moment.
	//this->aAttach->setEnabled(true);
#endif /* _WIN32 */

	QSettings settings;
	if (settings.contains("MainWinState")) {
		this->restoreState(settings.value("MainWinState").toByteArray());
	}
}

MainWindow::~MainWindow()
{
	/* Stop still running progs */
	UT_NOTIFY(LV_TRACE, "~MainWindow kill program");
	killProgram(1);

	/* Free reachable memory */
	UT_NOTIFY(LV_TRACE, "~MainWindow free pc");
	delete pc;
	delete m_pGlCallSt;
	delete m_pGlExtSt;
	delete m_pGlCallPfst;
	delete m_pGlExtPfst;
	delete m_pGlxCallSt;
	delete m_pGlxExtSt;
	delete m_pGlxCallPfst;
	delete m_pGlxExtPfst;
	delete m_pWglCallSt;
	delete m_pWglExtSt;
	delete m_pWglCallPfst;
	delete m_pWglExtPfst;

	delete m_pCurrentCall;
}

void MainWindow::killProgram(int hard)
{
	UT_NOTIFY(LV_TRACE, "Killing debugee");
	pc->killProgram(hard);
	/* status log */
	if (hard) {
		setStatusBarText(QString("Program termination forced!"));
		addGlTraceWarningItem("Program termination forced!");
	} else {
		setStatusBarText(QString("Program terminated!"));
		addGlTraceWarningItem("Program terminated!");
	}
}

void MainWindow::on_aQuit_triggered()
{
	UT_NOTIFY(LV_TRACE, "Quitting application");
	close();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	QSettings settings;
	settings.setValue("MainWinState", this->saveState());
	killProgram(1);
	//qApp->quit();
	event->accept();
}

void MainWindow::on_aOpen_triggered()
{
	Dialog_OpenProgram *dOpenProgram = new Dialog_OpenProgram();
	int i;

	/* Cleanup shader debugging */
	emit cleanShader();

	if (dbgProgArgs.size() != 0) {
		QString arguments = QString("");

		dOpenProgram->leProgram->setText(dbgProgArgs[0]);

		for (i = 1; i < dbgProgArgs.size(); i++) {
			arguments.append(dbgProgArgs[i]);
			if (i < dbgProgArgs.size() - 1) {
				arguments.append(" ");
			}
		}
		dOpenProgram->leArguments->setText(arguments);

		dOpenProgram->leWorkDir->setText(this->workDir);

	} else {
		/* Set MRU program, if no old value was available. */
		QString program, arguments, workDir;

		if (this->loadMruProgram(program, arguments, workDir)) {
			dOpenProgram->leProgram->setText(program);
			dOpenProgram->leArguments->setText(arguments);
			dOpenProgram->leWorkDir->setText(workDir);
		}
	}

	dOpenProgram->exec();

	if (dOpenProgram->result() == QDialog::Accepted) {
		if (!(dOpenProgram->leProgram->text().isEmpty())) {
			dbgProgArgs.clear();
			dbgProgArgs.append(dOpenProgram->leProgram->text());
			dbgProgArgs += dOpenProgram->leArguments->text().split(
					QRegExp("\\s+"), QString::SkipEmptyParts);
			/* cleanup dbg state */
			leaveDBGState();
			/* kill program */
			killProgram(1);
			setRunLevel(RL_SETUP);
			setErrorStatus(PCE_NONE);
			setStatusBarText(
					QString("New program: ") + dOpenProgram->leProgram->text());
			clearGlTraceItemList();
		}

		if (!dOpenProgram->leWorkDir->text().isEmpty()) {
			this->workDir = dOpenProgram->leWorkDir->text();
		} else {
			this->workDir.clear();
		}

		/* Save MRU program. */
		this->saveMruProgram(dOpenProgram->leProgram->text(),
				dOpenProgram->leArguments->text(),
				dOpenProgram->leWorkDir->text());
	}
	delete dOpenProgram;
}

void MainWindow::on_aAttach_triggered()
{
	UT_NOTIFY(LV_TRACE, "Quitting application");

	pcErrorCode errorCode;
	Dialog_AttachToProcess dlgAttach(this);
	dlgAttach.exec();

	if (dlgAttach.result() == QDialog::Accepted) {
		ProcessSnapshotModel::Item *process = dlgAttach.getSelectedItem();

		if ((process != NULL) && process->IsAttachtable()) {
			this->dbgProgArgs.clear();
			this->dbgProgArgs.append(QString(process->GetExe()));
			/* cleanup dbg state */
			this->leaveDBGState();
			/* kill program */
			this->killProgram(1);
			this->setRunLevel(RL_SETUP);
			this->setErrorStatus(PCE_NONE);
			this->setStatusBarText(
					QString("New program: ") + QString(process->GetExe()));
			this->clearGlTraceItemList();

			/* Attach to programm. */
			errorCode = this->pc->attachToProgram(process->GetPid());
			if (errorCode != PCE_NONE) {
				QMessageBox::critical(this, "Error", "Attaching to process "
						"failed.");
				this->setRunLevel(RL_INIT);
				this->setErrorStatus(errorCode);
			} else {
				//this->setRunLevel(RL_SETUP);
				this->setErrorStatus(PCE_NONE);
			}
		}
	}
}

void MainWindow::on_aOnlineHelp_triggered()
{
	QUrl url("http://www.vis.uni-stuttgart.de/glsldevil/");
	QDesktopServices ds;
	ds.openUrl(url);
}

void MainWindow::on_aAbout_triggered()
{
	Dialog_AboutBox dlg(this);
	dlg.exec();
}

void MainWindow::setGlStatisticTabs(int n, int m)
{
	while (twGlStatistics->count() > 0) {
		twGlStatistics->removeTab(0);
	}

	switch (n) {
	case 0:
		switch (m) {
		case 0:
			twGlStatistics->insertTab(0, taGlCalls, QString("GL Calls"));
			twGlStatistics->insertTab(1, taGlExt, QString("GL Extensions"));
			break;
		case 1:
			twGlStatistics->insertTab(0, taGlCallsPf, QString("GL Calls"));
			twGlStatistics->insertTab(1, taGlExtPf, QString("GL Extensions"));
			break;
		}
		break;
	case 1:
		switch (m) {
		case 0:
			twGlStatistics->insertTab(0, taGlxCalls, QString("GLX Calls"));
			twGlStatistics->insertTab(1, taGlxExt, QString("GLX Extensions"));
			break;
		case 1:
			twGlStatistics->insertTab(0, taGlxCallsPf, QString("GLX Calls"));
			twGlStatistics->insertTab(1, taGlxExtPf, QString("GLX Extensions"));
			break;
		}
		break;
	case 2:
		switch (m) {
		case 0:
			twGlStatistics->insertTab(0, taWglCalls, QString("WGL Calls"));
			twGlStatistics->insertTab(1, taWglExt, QString("WGL Extensions"));
			break;
		case 1:
			twGlStatistics->insertTab(0, taWglCallsPf, QString("WGL Calls"));
			twGlStatistics->insertTab(1, taWglExtPf, QString("WGL Extensions"));
			break;
		}
		break;
	default:
		break;
	}
}

void MainWindow::on_tbBVCapture_clicked()
{
	int width, height;
	float *imageData;
	pcErrorCode error;

	error = pc->readBackActiveRenderBuffer(3, &width, &height, &imageData);

	if (error == PCE_NONE) {
		PixelBox imageBox;
		imageBox.setData<float>(width, height, 3, imageData);
		lBVLabel->setPixmap(
				QPixmap::fromImage(imageBox.getByteImage(PixelBox::FBM_CLAMP)));
		lBVLabel->resize(width, height);
		tbBVSave->setEnabled(true);
	} else {
		setErrorStatus(error);
	}
}

void MainWindow::on_tbBVCaptureAutomatic_toggled(bool b)
{
	if (b) {
		tbBVCapture->setEnabled(false);
		on_tbBVCapture_clicked();
	} else {
		tbBVCapture->setEnabled(true);
	}
}

void MainWindow::on_tbBVSave_clicked()
{
	static QStringList history;
	static QDir directory = QDir::current();

	if (lBVLabel->pixmap()) {
		QImage img = lBVLabel->pixmap()->toImage();
		if (!img.isNull()) {
			QFileDialog *sDialog = new QFileDialog(this, QString("Save image"));

			sDialog->setAcceptMode(QFileDialog::AcceptSave);
			sDialog->setFileMode(QFileDialog::AnyFile);
			QStringList formatDesc;
			formatDesc << "Portable Network Graphics (*.png)"
					<< "Windows Bitmap (*.bmp)"
					<< "Joint Photographic Experts Group (*.jpg, *.jepg)"
					<< "Portable Pixmap (*.ppm)"
					<< "Tagged Image File Format (*.tif, *.tiff)"
					<< "X11 Bitmap (*.xbm, *.xpm)";
			sDialog->setFilters(formatDesc);

			if (!(history.isEmpty())) {
				sDialog->setHistory(history);
			}

			sDialog->setDirectory(directory);

			if (sDialog->exec()) {
				QStringList files = sDialog->selectedFiles();
				QString selected;
				if (!files.isEmpty()) {
					selected = files[0];
					if (!(img.save(selected))) {

						QString forceFilter;
						QString filter = sDialog->selectedFilter();
						if (filter
								== QString(
										"Portable Network Graphics (*.png)")) {
							forceFilter.append("png");
						} else if (filter
								== QString("Windows Bitmap (*.bmp)")) {
							forceFilter.append("bmp");
						} else if (filter
								== QString(
										"Joint Photographic Experts Group (*.jpg, *.jepg)")) {
							forceFilter.append("jpg");
						} else if (filter
								== QString("Portable Pixmap (*.ppm)")) {
							forceFilter.append("ppm");
						} else if (filter
								== QString(
										"Tagged Image File Format (*.tif, *.tiff)")) {
							forceFilter.append("tif");
						} else if (filter
								== QString("X11 Bitmap (*.xbm, *.xpm)")) {
							forceFilter.append("xbm");
						}

						img.save(selected, forceFilter.toLatin1().data());
					}
				}
			}

			history = sDialog->history();
			directory = sDialog->directory();

			delete sDialog;
		}
	}
}

void MainWindow::on_cbGlstCallOrigin_currentIndexChanged(int n)
{
	setGlStatisticTabs(n, cbGlstPfMode->currentIndex());
}

void MainWindow::on_cbGlstPfMode_currentIndexChanged(int m)
{
	setGlStatisticTabs(cbGlstCallOrigin->currentIndex(), m);
}

pcErrorCode MainWindow::getNextCall()
{
	m_pCurrentCall = pc->getCurrentCall();
	if (!shaderManager->isAvaliable() && m_pCurrentCall->isDebuggableDrawCall()) {
		/* current call is a drawcall and we don't have valid shader code;
		 * call debug function that reads back the shader code
		 */
		int status;
		emit updateShaders(status);
		pcErrorCode error = static_cast<pcErrorCode>(status);
		if (isErrorCritical(error))
			return error;
	}
	return PCE_NONE;
}

void MainWindow::on_tbExecute_clicked()
{
	UT_NOTIFY(LV_TRACE, "Executing");
	delete m_pCurrentCall;
	m_pCurrentCall = NULL;

	/* Cleanup shader debugging */
	emit cleanShader();

	if (currentRunLevel == RL_SETUP) {
		int i;
		char **args;
		char *workDir;

		/* Clear previous status */
		setStatusBarText(QString(""));
		clearGlTraceItemList();
		resetAllStatistics();

		emit removeShaders();

		/* Build arguments */
		args = new char*[dbgProgArgs.size() + 1];
		for (i = 0; i < dbgProgArgs.size(); i++) {
			args[i] = strdup(dbgProgArgs[i].toAscii().data());
		}
		//args[dbgProgArgs.size()] = (char*)malloc(sizeof(char));
		//args[dbgProgArgs.size()] = '\0';
		args[dbgProgArgs.size()] = 0;

		if (this->workDir.isEmpty()) {
			workDir = NULL;
		} else {
			workDir = strdup(this->workDir.toAscii().data());
		}

		/* Execute prog */
		pcErrorCode error = pc->runProgram(args, workDir);

		/* Error handling */
		setErrorStatus(error);
		if (error != PCE_NONE) {
			setRunLevel(RL_SETUP);
		} else {
			setStatusBarText(QString("Executing " + dbgProgArgs[0]));
			m_pCurrentCall = pc->getCurrentCall();
			setRunLevel(RL_TRACE_EXECUTE);
			addGlTraceWarningItem("Program Start");
			addGlTraceItem();
		}

		/* Clean up */
		for (i = 0; i < dbgProgArgs.size() + 1; i++) {
			free(args[i]);
		}
		delete[] args;
		free(workDir);
	} else {
		/* cleanup dbg state */
		leaveDBGState();

		/* Stop already running progs */
		killProgram(1);

		setRunLevel(RL_SETUP);
	}
}

void MainWindow::addGlTraceItem()
{
	if (!m_pCurrentCall)
		return;

	char *callString = m_pCurrentCall->getCallString();
	QIcon icon;
	QString iconText;
	GlTraceListItem::IconType iconType;

	if (currentRunLevel == RL_TRACE_EXECUTE_NO_DEBUGABLE
			|| currentRunLevel == RL_TRACE_EXECUTE_IS_DEBUGABLE) {
		iconType = GlTraceListItem::IT_ACTUAL;
	} else if (currentRunLevel == RL_TRACE_EXECUTE_RUN) {
		iconType = GlTraceListItem::IT_OK;
	} else {
		iconType = GlTraceListItem::IT_EMPTY;
	}

	if (m_pCurrentCall->isGlFunc()) {
		m_pGlCallSt->incCallStatistic(QString(m_pCurrentCall->getName()));
		m_pGlExtSt->incCallStatistic(QString(m_pCurrentCall->getExtension()));
		m_pGlCallPfst->incCallStatistic(QString(m_pCurrentCall->getName()));
		m_pGlExtPfst->incCallStatistic(QString(m_pCurrentCall->getExtension()));
	} else if (m_pCurrentCall->isGlxFunc()) {
		m_pGlxCallSt->incCallStatistic(QString(m_pCurrentCall->getName()));
		m_pGlxExtSt->incCallStatistic(QString(m_pCurrentCall->getExtension()));
		m_pGlxCallPfst->incCallStatistic(QString(m_pCurrentCall->getName()));
		m_pGlxExtPfst->incCallStatistic(
				QString(m_pCurrentCall->getExtension()));
	} else if (m_pCurrentCall->isWglFunc()) {
		m_pWglCallSt->incCallStatistic(QString(m_pCurrentCall->getName()));
		m_pWglExtSt->incCallStatistic(QString(m_pCurrentCall->getExtension()));
		m_pWglCallPfst->incCallStatistic(QString(m_pCurrentCall->getName()));
		m_pWglExtPfst->incCallStatistic(
				QString(m_pCurrentCall->getExtension()));
	}

	/* Check what options are valid depending on the command */
	if (m_pCurrentCall->isDebuggable()) {

	}

	if (m_pGlTraceModel) {
		m_pGlTraceModel->addGlTraceItem(iconType, callString);
	}
	lvGlTrace->scrollToBottom();
	free(callString);
}

void MainWindow::addGlTraceErrorItem(const char *text)
{
	if (m_pGlTraceModel) {
		m_pGlTraceModel->addGlTraceItem(GlTraceListItem::IT_ERROR, text);
	}
	lvGlTrace->scrollToBottom();
}

void MainWindow::addGlTraceWarningItem(const char *text)
{
	if (m_pGlTraceModel) {
		m_pGlTraceModel->addGlTraceItem(GlTraceListItem::IT_WARNING, text);
	}
	lvGlTrace->scrollToBottom();
}

void MainWindow::setGlTraceItemIconType(const GlTraceListItem::IconType type)
{
	if (m_pGlTraceModel) {
		m_pGlTraceModel->setCurrentGlTraceIconType(type, -1);
	}
}

void MainWindow::setGlTraceItemText(const char *text)
{
	if (m_pGlTraceModel) {
		m_pGlTraceModel->setCurrentGlTraceText(text, -1);
	}
}

void MainWindow::clearGlTraceItemList(void)
{
	if (m_pGlTraceModel) {
		m_pGlTraceModel->clear();
	}
}

pcErrorCode MainWindow::nextStep(const FunctionCall *fCall)
{
	pcErrorCode error;

	if ((fCall && fCall->isShaderSwitch())
			|| m_pCurrentCall->isShaderSwitch()) {
		/* current call is a glsl shader switch */

		/* call shader switch */
		error = pc->callOrigFunc(fCall);

		if (error != PCE_NONE) {
			if (isErrorCritical(error)) {
				return error;
			}
		} else {
			/* call debug function that reads back the shader code */
			int status;
			emit updateShaders(status);
			error = static_cast<pcErrorCode>(status);
			if (isErrorCritical(error))
				return error;
		}
	} else {
		/* current call is a "normal" function call */
		error = pc->callOrigFunc(fCall);
		if (isErrorCritical(error))
			return error;
	}
	/* Readback image if requested by user */
	if (tbBVCaptureAutomatic->isChecked()
			&& m_pCurrentCall->isFramebufferChange()) {
		on_tbBVCapture_clicked();
	}
	if (error == PCE_NONE) {
		error = pc->callDone();
	} else {
		/* TODO: what about the error code ??? */
		pc->callDone();
	}
	return error;
}

void MainWindow::on_tbStep_clicked()
{
	leaveDBGState();

	setRunLevel(RL_TRACE_EXECUTE_RUN);

	singleStep();

	if (currentRunLevel != RL_TRACE_EXECUTE_RUN) {
		// a critical error occured in step */
		return;
	}

	while (currentRunLevel == RL_TRACE_EXECUTE_RUN
			&& !m_pGlTraceFilterModel->isFunctionVisible(
					m_pCurrentCall->getName())) {
		singleStep();
		qApp->processEvents(QEventLoop::AllEvents);
		if (currentRunLevel == RL_SETUP) {
			/* something was wrong in step */
			return;
		}
	}

	setRunLevel(RL_TRACE_EXECUTE);
	setGlTraceItemIconType(GlTraceListItem::IT_ACTUAL);
}

void MainWindow::singleStep()
{
	resetPerFrameStatistics();

	/* cleanup dbg state */
	leaveDBGState();

	setGlTraceItemIconType(GlTraceListItem::IT_OK);

	pcErrorCode error = nextStep(NULL);

	delete m_pCurrentCall;
	m_pCurrentCall = NULL;

	/* Error handling */
	setErrorStatus(error);
	if (isErrorCritical(error)) {
		killProgram(1);
		setRunLevel(RL_SETUP);
		return;
	} else {
		if (currentRunLevel == RL_TRACE_EXECUTE_RUN && isOpenGLError(error)
				&& tbToggleHaltOnError->isChecked()) {
			setRunLevel(RL_TRACE_EXECUTE);
		}
		error = getNextCall();
		setErrorStatus(error);
		if (isErrorCritical(error)) {
			killProgram(1);
			setRunLevel(RL_SETUP);
			return;
		}
		if (currentRunLevel != RL_TRACE_EXECUTE_RUN) {
			setRunLevel(RL_TRACE_EXECUTE);
		}
		addGlTraceItem();
	}

}

void MainWindow::saveQueries(int &error)
{
	if (currentRunLevel == RL_TRACE_EXECUTE_IS_DEBUGABLE)
		error = pc->saveAndInterruptQueries();
	else
		error = PCE_NONE;
}

void MainWindow::recordDrawCall(int &error)
{
	/* record stream, store currently active shader program,
	 * and enter debug state only when shader is executed the first time
	 * and not after restart.
	 */
	error = PCE_NONE;
	if (currentRunLevel != RL_TRACE_EXECUTE_IS_DEBUGABLE)
		return;

	setRunLevel(RL_DBG_RECORD_DRAWCALL);
	/* save active shader */
	error = pc->saveActiveShader();
	if (isErrorCritical((pcErrorCode)error))
		return;

	recordDrawCall();
	/* has the user interrupted the recording? */
	if (currentRunLevel != RL_DBG_RECORD_DRAWCALL)
		error = PCE_RETURN;
}

void MainWindow::on_tbSkip_clicked()
{
	resetPerFrameStatistics();

	/* cleanup dbg state */
	leaveDBGState();

	setGlTraceItemIconType(GlTraceListItem::IT_ERROR);

	pcErrorCode error = pc->callDone();

	delete m_pCurrentCall;
	m_pCurrentCall = NULL;

	/* Error handling */
	setErrorStatus(error);
	if (isErrorCritical(error)) {
		killProgram(1);
		setRunLevel(RL_SETUP);
	} else {
		error = getNextCall();
		setErrorStatus(error);
		if (isErrorCritical(error)) {
			killProgram(1);
			setRunLevel(RL_SETUP);
			return;
		}
		setRunLevel(RL_TRACE_EXECUTE);
		addGlTraceItem();
	}
}

void MainWindow::on_tbEdit_clicked()
{
	EditCallDialog *dialog = new EditCallDialog(m_pCurrentCall);
	dialog->exec();

	if (dialog->result() == QDialog::Accepted) {
		const FunctionCall *editCall = dialog->getChangedFunction();

		resetPerFrameStatistics();

		if ((*(FunctionCall*) editCall) != *m_pCurrentCall) {

			char *callString = editCall->getCallString();
			setGlTraceItemText(callString);
			setGlTraceItemIconType(GlTraceListItem::IT_IMPORTANT);
			free(callString);

			/* Send data to debug library */
			pc->overwriteFuncArguments(editCall);

			/* Replace current call by edited call */
			delete m_pCurrentCall;
			m_pCurrentCall = NULL;

			m_pCurrentCall = new FunctionCall(editCall);
			delete editCall;

		} else {
			/* Just cleanup */
			delete editCall;
		}
	}

	delete dialog;
}

void MainWindow::waitForEndOfExecution()
{
	pcErrorCode error;
	while (currentRunLevel == RL_TRACE_EXECUTE_RUN) {
		qApp->processEvents(QEventLoop::AllEvents);
#ifndef _WIN32
		usleep(1000);
#else /* !_WIN32 */
		Sleep(1);
#endif /* !_WIN32 */
		int state;
		error = pc->checkExecuteState(&state);
		if (isErrorCritical(error)) {
			killProgram(1);
			setRunLevel(RL_SETUP);
			return;
		} else if (isOpenGLError(error)) {
			/* TODO: error check */
			pc->executeContinueOnError();
			pc->checkChildStatus();
			m_pCurrentCall = pc->getCurrentCall();
			addGlTraceItem();
			setErrorStatus(error);
			pc->callDone();
			error = getNextCall();
			setErrorStatus(error);
			if (isErrorCritical(error)) {
				killProgram(1);
				setRunLevel(RL_SETUP);
				return;
			} else {
				setRunLevel(RL_TRACE_EXECUTE);
				addGlTraceItem();
			}
			break;
		}
		if (state) {
			break;
		}
		if (!pc->childAlive()) {
			UT_NOTIFY(LV_INFO, "Debugee terminated!");
			killProgram(0);
			setRunLevel(RL_SETUP);
			return;
		}
	}
	if (currentRunLevel == RL_TRACE_EXECUTE_RUN) {
		pc->checkChildStatus();
		error = getNextCall();
		setRunLevel(RL_TRACE_EXECUTE);
		setErrorStatus(error);
		if (isErrorCritical(error)) {
			killProgram(1);
			setRunLevel(RL_SETUP);
			return;
		} else {
			addGlTraceItem();
		}
	}
}

void MainWindow::on_tbJumpToDrawCall_clicked()
{
	/* cleanup dbg state */
	leaveDBGState();

	setRunLevel(RL_TRACE_EXECUTE_RUN);

	if (tbToggleNoTrace->isChecked()) {
		delete m_pCurrentCall;
		m_pCurrentCall = NULL;

		emit removeShaders();

		resetAllStatistics();
		setStatusBarText(QString("Running program without tracing"));
		clearGlTraceItemList();
		addGlTraceWarningItem("Running program without call tracing");
		addGlTraceWarningItem("Statistics reset!");
		qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

		pcErrorCode error = pc->executeToDrawCall(
				tbToggleHaltOnError->isChecked());
		setErrorStatus(error);
		if (error != PCE_NONE) {
			killProgram(1);
			setRunLevel(RL_SETUP);
			return;
		}
		waitForEndOfExecution(); /* last statement!! */
	} else {
		if (m_pCurrentCall && m_pCurrentCall->isDebuggableDrawCall()) {
			singleStep();
			if (currentRunLevel == RL_SETUP) {
				/* something was wrong in step */
				return;
			}
		}

		while (currentRunLevel == RL_TRACE_EXECUTE_RUN
				&& (!m_pCurrentCall
						|| (m_pCurrentCall
								&& !m_pCurrentCall->isDebuggableDrawCall()))) {
			singleStep();
			if (currentRunLevel == RL_SETUP) {
				/* something was wrong in step */
				return;
			}
			qApp->processEvents(QEventLoop::AllEvents);
		}
	}

	setRunLevel(RL_TRACE_EXECUTE);
	setGlTraceItemIconType(GlTraceListItem::IT_ACTUAL);
}

void MainWindow::on_tbJumpToShader_clicked()
{
	/* cleanup dbg state */
	leaveDBGState();

	setRunLevel(RL_TRACE_EXECUTE_RUN);

	if (tbToggleNoTrace->isChecked()) {
		delete m_pCurrentCall;
		m_pCurrentCall = NULL;

		emit removeShaders();

		resetAllStatistics();
		setStatusBarText(QString("Running program without tracing"));
		clearGlTraceItemList();
		addGlTraceWarningItem("Running program without call tracing");
		addGlTraceWarningItem("Statistics reset!");
		qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

		pcErrorCode error = pc->executeToShaderSwitch(
				tbToggleHaltOnError->isChecked());
		setErrorStatus(error);
		if (error != PCE_NONE) {
			killProgram(1);
			setRunLevel(RL_SETUP);
			return;
		}
		waitForEndOfExecution(); /* last statement!! */
	} else {
		if (m_pCurrentCall && m_pCurrentCall->isShaderSwitch()) {
			singleStep();
			/* something was wrong in step */
			if (currentRunLevel == RL_SETUP)
				return;
		}

		while (currentRunLevel == RL_TRACE_EXECUTE_RUN
				&& (!m_pCurrentCall
						|| (m_pCurrentCall && !m_pCurrentCall->isShaderSwitch()))) {
			singleStep();
			/* something was wrong in step */
			if (currentRunLevel == RL_SETUP)
				return;
			qApp->processEvents(QEventLoop::AllEvents);
		}
	}

	setRunLevel(RL_TRACE_EXECUTE);
	setGlTraceItemIconType(GlTraceListItem::IT_ACTUAL);
}

void MainWindow::on_tbJumpToUserDef_clicked()
{
	static QString targetName;
	JumpToDialog *pJumpToDialog = new JumpToDialog(targetName);

	pJumpToDialog->exec();

	if (pJumpToDialog->result() == QDialog::Accepted) {
		/* cleanup dbg state */
		leaveDBGState();

		setRunLevel(RL_TRACE_EXECUTE_RUN);
		targetName = pJumpToDialog->getTargetFuncName();

		if (tbToggleNoTrace->isChecked()) {
			delete m_pCurrentCall;
			m_pCurrentCall = NULL;

			emit removeShaders();
			resetAllStatistics();
			setStatusBarText(QString("Running program without tracing"));
			clearGlTraceItemList();
			addGlTraceWarningItem("Running program without call tracing");
			addGlTraceWarningItem("Statistics reset!");
			qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

			pcErrorCode error = pc->executeToUserDefined(
					targetName.toAscii().data(),
					tbToggleHaltOnError->isChecked());
			setErrorStatus(error);
			if (error != PCE_NONE) {
				killProgram(1);
				setRunLevel(RL_SETUP);
				return;
			}
			waitForEndOfExecution(); /* last statement!! */
		} else {

			singleStep();
			/* something was wrong in step */
			if (currentRunLevel == RL_SETUP)
				return;

			while (currentRunLevel == RL_TRACE_EXECUTE_RUN
					&& (!m_pCurrentCall
							|| (m_pCurrentCall
									&& targetName.compare(
											m_pCurrentCall->getName())))) {
				singleStep();
				/* something was wrong in step */
				if (currentRunLevel == RL_SETUP)
					return;
				qApp->processEvents(QEventLoop::AllEvents);
			}
		}

		setRunLevel(RL_TRACE_EXECUTE);
		setGlTraceItemIconType(GlTraceListItem::IT_ACTUAL);
	}

	delete pJumpToDialog;
}

void MainWindow::on_tbRun_clicked()
{
	/* cleanup dbg state */
	leaveDBGState();

	setRunLevel(RL_TRACE_EXECUTE_RUN);
	if (tbToggleNoTrace->isChecked()) {
		delete m_pCurrentCall;
		m_pCurrentCall = NULL;

		emit removeShaders();
		resetAllStatistics();
		setStatusBarText(QString("Running program without tracing"));
		clearGlTraceItemList();
		addGlTraceWarningItem("Running program without call tracing");
		addGlTraceWarningItem("Statistics reset!");
		qApp->processEvents(QEventLoop::ExcludeUserInputEvents);

		pcErrorCode error = pc->execute(tbToggleHaltOnError->isChecked());
		setErrorStatus(error);
		if (isErrorCritical(error)) {
			killProgram(1);
			setRunLevel(RL_SETUP);
			return;
		}
		waitForEndOfExecution(); /* last statement !! */
	} else {
		while (currentRunLevel == RL_TRACE_EXECUTE_RUN) {
			singleStep();
			/* something was wrong in step */
			if (currentRunLevel == RL_SETUP)
				return;
			qApp->processEvents(QEventLoop::AllEvents);
		}
		setGlTraceItemIconType(GlTraceListItem::IT_ACTUAL);
	}
}

void MainWindow::on_tbPause_clicked()
{
	if (tbToggleNoTrace->isChecked()) {
		pc->stop();
	} else {
		setRunLevel(RL_TRACE_EXECUTE);
	}
}

void MainWindow::setGuiUpdates(bool enabled)
{
	lvGlTrace->setUpdatesEnabled(enabled);
	tvGlCalls->setUpdatesEnabled(enabled);
	tvGlCallsPf->setUpdatesEnabled(enabled);
	tvGlExt->setUpdatesEnabled(enabled);
	tvGlExtPf->setUpdatesEnabled(enabled);
	tvGlxCalls->setUpdatesEnabled(enabled);
	tvGlxCallsPf->setUpdatesEnabled(enabled);
	tvGlxExt->setUpdatesEnabled(enabled);
	tvGlxExtPf->setUpdatesEnabled(enabled);
	tvWglCalls->setUpdatesEnabled(enabled);
	tvWglCallsPf->setUpdatesEnabled(enabled);
	tvWglExt->setUpdatesEnabled(enabled);
	tvWglExtPf->setUpdatesEnabled(enabled);
}

void MainWindow::on_tbToggleGuiUpdate_clicked(bool b)
{
	setGuiUpdates(!b);
}

void MainWindow::on_tbToggleNoTrace_clicked(bool)
{
}

void MainWindow::on_tbToggleHaltOnError_clicked(bool)
{
}

void MainWindow::on_tbGlTraceSettings_clicked()
{
	m_pgtDialog->exec();
	m_pGlTraceModel->resetLayout();
}

void MainWindow::on_tbSave_clicked()
{

	static QStringList history;
	static QDir directory = QDir::current();

	QFileDialog *sDialog = new QFileDialog(this, QString("Save GL Trace as"));

	sDialog->setAcceptMode(QFileDialog::AcceptSave);
	sDialog->setFileMode(QFileDialog::AnyFile);
	QStringList formatDesc;
	formatDesc << "Plain Text (*.txt)";
	sDialog->setFilters(formatDesc);

	if (!(history.isEmpty())) {
		sDialog->setHistory(history);
	}

	sDialog->setDirectory(directory);

	if (sDialog->exec()) {
		QStringList files = sDialog->selectedFiles();
		QString selected;
		if (!files.isEmpty()) {
			QString filter = sDialog->selectedFilter();
			if (filter == QString("Plain Text (*.txt)")) {
				QFile file(files[0]);
				if (file.open(QIODevice::WriteOnly)) {

					QTextStream out(&file);
					m_pGlTraceModel->traverse(out, &GlTraceListItem::outputTXT);
				}
			}
		}
	}

	history = sDialog->history();
	directory = sDialog->directory();

	delete sDialog;
}

pcErrorCode MainWindow::recordCall()
{
	resetPerFrameStatistics();

	setGlTraceItemIconType(GlTraceListItem::IT_RECORD);

	pcErrorCode error = pc->recordCall();
	/* TODO: error handling!!!!!! */

	error = pc->callDone();

	delete m_pCurrentCall;
	m_pCurrentCall = NULL;

	/* Error handling */
	setErrorStatus(error);
	if (isErrorCritical(error)) {
		killProgram(1);
		setRunLevel(RL_SETUP);
	} else {
		error = getNextCall();
		setErrorStatus(error);
		if (isErrorCritical(error)) {
			killProgram(1);
			setRunLevel(RL_SETUP);
		} else {
			addGlTraceItem();
		}
	}
	return error;
}

void MainWindow::recordDrawCall()
{
	pc->initRecording();
	if (!strcmp(m_pCurrentCall->getName(), "glBegin")) {
		while (currentRunLevel == RL_DBG_RECORD_DRAWCALL
				&& (!m_pCurrentCall
						|| (m_pCurrentCall
								&& strcmp(m_pCurrentCall->getName(), "glEnd")))) {
			if (recordCall() != PCE_NONE) {
				return;
			}
			qApp->processEvents(QEventLoop::AllEvents);
		}
		if (!m_pCurrentCall || strcmp(m_pCurrentCall->getName(), "glEnd")
				|| recordCall() != PCE_NONE) {
			if (currentRunLevel == RL_DBG_RECORD_DRAWCALL) {
				/* TODO: error handling */
				UT_NOTIFY(LV_WARN, "recordDrawCall: begin without end????");
				return;
			} else {
				/* draw call recording stopped by user interaction */
				pcErrorCode error = pc->insertGlEnd();
				if (error == PCE_NONE) {
					int target = shaderManager->getCurrentTarget();
					error = PCE_DBG_INVALID_VALUE;
					if (target >= 0)
						error = pc->restoreRenderTarget(target);
				}
				if (error == PCE_NONE)
					error = pc->restoreActiveShader();
				if (error == PCE_NONE)
					error = pc->endReplay();
				setErrorStatus(error);
				if (isErrorCritical(error)) {
					killProgram(1);
					setRunLevel(RL_SETUP);
					return;
				}
				setGlTraceItemIconType(GlTraceListItem::IT_ACTUAL);
				return;
			}
		}
		setGlTraceItemIconType(GlTraceListItem::IT_ACTUAL);
	} else {
		if (recordCall() != PCE_NONE) {
			return;
		}
	}
}


void MainWindow::leaveDBGState()
{
	pcErrorCode error = PCE_NONE;

	switch (currentRunLevel) {
	case RL_DBG_RESTART:
	case RL_DBG_RECORD_DRAWCALL:
		/* restore shader program */
		error = pc->restoreActiveShader();
		setErrorStatus(error);
		if (isErrorCritical(error)) {
			killProgram(1);
			setRunLevel(RL_SETUP);
			return;
		}
		error = pc->restartQueries();
		setErrorStatus(error);
		if (isErrorCritical(error)) {
			killProgram(1);
			setRunLevel(RL_SETUP);
			return;
		}
		error = pc->endReplay();
		setErrorStatus(error);
		if (isErrorCritical(error)) {
			killProgram(1);
			setRunLevel(RL_SETUP);
			return;
		}
		/* TODO: close all windows (obsolete?) */
		break;
	default:
		break;
	}
}

void MainWindow::setRunLevel(int rl)
{
	QString title = QString(MAIN_WINDOW_TITLE);
	UT_NOTIFY(LV_INFO,
			"new level: " << rl << " " << (m_pCurrentCall ? m_pCurrentCall->getName() : ""));

	switch (rl) {
	case RL_INIT:  // Program start
		currentRunLevel = RL_INIT;
		this->setWindowTitle(title);
		aOpen->setEnabled(true);
#ifdef _WIN32
		//aAttach->setEnabled(true);
		// TODO
#endif /* _WIN32 */
		tbBVCaptureAutomatic->setEnabled(false);
		tbBVCaptureAutomatic->setChecked(false);
		tbBVCapture->setEnabled(false);
		tbBVSave->setEnabled(false);
		lBVLabel->setPixmap(QPixmap());
		tbExecute->setEnabled(false);
		tbExecute->setIcon(
				QIcon(
						QString::fromUtf8(
								":/icons/icons/gltrace-execute_32.png")));
		tbStep->setEnabled(false);
		tbSkip->setEnabled(false);
		tbEdit->setEnabled(false);
		tbJumpToDrawCall->setEnabled(false);
		tbJumpToShader->setEnabled(false);
		tbJumpToUserDef->setEnabled(false);
		tbRun->setEnabled(false);
		tbPause->setEnabled(false);
		tbToggleGuiUpdate->setEnabled(false);
		tbToggleNoTrace->setEnabled(false);
		tbToggleHaltOnError->setEnabled(false);
		tbGlTraceSettings->setEnabled(false);
		tbSave->setEnabled(false);
		setGuiUpdates(true);
		break;
	case RL_SETUP:  // User has setup parameters for debugging
		currentRunLevel = RL_SETUP;
		title.append(" - ");
		title.append(dbgProgArgs[0]);
		this->setWindowTitle(title);
		aOpen->setEnabled(true);
#ifdef _WIN32
		//aAttach->setEnabled(true);
		// TODO
#endif /* _WIN32 */
		tbBVCapture->setEnabled(false);
		tbBVCaptureAutomatic->setEnabled(false);
		tbBVCaptureAutomatic->setChecked(false);
		tbBVSave->setEnabled(false);
		lBVLabel->setPixmap(QPixmap());
		tbExecute->setEnabled(true);
		tbExecute->setIcon(
				QIcon(
						QString::fromUtf8(
								":/icons/icons/gltrace-execute_32.png")));
		tbStep->setEnabled(false);
		tbSkip->setEnabled(false);
		tbEdit->setEnabled(false);
		tbJumpToDrawCall->setEnabled(false);
		tbJumpToShader->setEnabled(false);
		tbJumpToUserDef->setEnabled(false);
		tbRun->setEnabled(false);
		tbPause->setEnabled(false);
		tbToggleGuiUpdate->setEnabled(false);
		tbToggleNoTrace->setEnabled(false);
		tbToggleHaltOnError->setEnabled(false);
		tbGlTraceSettings->setEnabled(true);
		tbSave->setEnabled(true);
		setGuiUpdates(true);
		break;
	case RL_TRACE_EXECUTE:  // Trace is running in step mode
		/* choose sub-level */
		break;
	case RL_TRACE_EXECUTE_NO_DEBUGABLE:  // sub-level for non debugable calls
		currentRunLevel = RL_TRACE_EXECUTE_NO_DEBUGABLE;
		title.append(" - ");
		title.append(dbgProgArgs[0]);
		this->setWindowTitle(title);
		aOpen->setEnabled(true);
#ifdef _WIN32
		//aAttach->setEnabled(true);
		// TODO
#endif /* _WIN32 */
		if (!tbBVCaptureAutomatic->isChecked()) {
			tbBVCapture->setEnabled(true);
		}
		tbBVCaptureAutomatic->setEnabled(true);
		tbExecute->setEnabled(true);
		tbExecute->setIcon(
				QIcon(
						QString::fromUtf8(
								":/icons/icons/face-devil-green-grin_32.png")));
		tbStep->setEnabled(true);
		tbSkip->setEnabled(true);
		if (m_pCurrentCall && m_pCurrentCall->isEditable()) {
			tbEdit->setEnabled(true);
		} else {
			tbEdit->setEnabled(false);
		}
		tbJumpToDrawCall->setEnabled(true);
		tbJumpToShader->setEnabled(true);
		tbJumpToUserDef->setEnabled(true);
		tbRun->setEnabled(true);
		tbPause->setEnabled(false);
		tbToggleGuiUpdate->setEnabled(true);
		tbToggleNoTrace->setEnabled(true);
		tbToggleHaltOnError->setEnabled(true);
		tbGlTraceSettings->setEnabled(true);
		tbSave->setEnabled(true);
		setGuiUpdates(true);
		break;
	case RL_TRACE_EXECUTE_IS_DEBUGABLE:  // sub-level for debugable calls
		currentRunLevel = RL_TRACE_EXECUTE_IS_DEBUGABLE;
		title.append(" - ");
		title.append(dbgProgArgs[0]);
		this->setWindowTitle(title);
		aOpen->setEnabled(true);
#ifdef _WIN32
		//aAttach->setEnabled(true);
		// TODO
#endif /* _WIN32 */
		if (!tbBVCaptureAutomatic->isChecked()) {
			tbBVCapture->setEnabled(true);
		}
		tbBVCaptureAutomatic->setEnabled(true);
		tbExecute->setEnabled(true);
		tbExecute->setIcon(
				QIcon(
						QString::fromUtf8(
								":/icons/icons/face-devil-green-grin_32.png")));
		tbStep->setEnabled(true);
		tbSkip->setEnabled(true);
		if (m_pCurrentCall && m_pCurrentCall->isEditable()) {
			tbEdit->setEnabled(true);
		} else {
			tbEdit->setEnabled(false);
		}
		tbJumpToDrawCall->setEnabled(true);
		tbJumpToShader->setEnabled(true);
		tbJumpToUserDef->setEnabled(true);
		tbRun->setEnabled(true);
		tbPause->setEnabled(false);
		tbToggleGuiUpdate->setEnabled(true);
		tbToggleNoTrace->setEnabled(true);
		tbToggleHaltOnError->setEnabled(true);
		tbGlTraceSettings->setEnabled(true);
		tbSave->setEnabled(true);
		setGuiUpdates(true);
		break;
	case RL_TRACE_EXECUTE_RUN:
		currentRunLevel = RL_TRACE_EXECUTE_RUN;
		title.append(" - ");
		title.append(dbgProgArgs[0]);
		this->setWindowTitle(title);
		aOpen->setEnabled(false);
		aAttach->setEnabled(false);
		if (!tbBVCaptureAutomatic->isChecked()) {
			tbBVCapture->setEnabled(true);
		}
		tbBVCaptureAutomatic->setEnabled(true);
		tbExecute->setEnabled(true);
		tbExecute->setIcon(
				QIcon(
						QString::fromUtf8(
								":/icons/icons/face-devil-green-grin_32.png")));
		tbStep->setEnabled(false);
		tbSkip->setEnabled(false);
		tbEdit->setEnabled(false);
		tbJumpToDrawCall->setEnabled(false);
		tbJumpToShader->setEnabled(false);
		tbJumpToUserDef->setEnabled(false);
		tbRun->setEnabled(false);
		tbPause->setEnabled(true);
		if (tbToggleNoTrace->isChecked()) {
			tbToggleGuiUpdate->setEnabled(false);
		} else {
			tbToggleGuiUpdate->setEnabled(true);
		}
		tbToggleNoTrace->setEnabled(false);
		tbToggleHaltOnError->setEnabled(false);
		tbGlTraceSettings->setEnabled(false);
		tbSave->setEnabled(false);
		/* update handling */
		setGuiUpdates(!tbToggleGuiUpdate->isChecked());
		break;
	case RL_DBG_RECORD_DRAWCALL:
		currentRunLevel = RL_DBG_RECORD_DRAWCALL;
		title.append(" - ");
		title.append(dbgProgArgs[0]);
		this->setWindowTitle(title);
		aOpen->setEnabled(false);
		aAttach->setEnabled(false);
		if (!tbBVCaptureAutomatic->isChecked()) {
			tbBVCapture->setEnabled(true);
		}
		tbBVCaptureAutomatic->setEnabled(true);
		tbExecute->setEnabled(true);
		tbExecute->setIcon(
				QIcon(
						QString::fromUtf8(
								":/icons/icons/face-devil-green-grin_32.png")));
		tbStep->setEnabled(false);
		tbSkip->setEnabled(false);
		tbEdit->setEnabled(false);
		tbJumpToDrawCall->setEnabled(false);
		tbJumpToShader->setEnabled(false);
		tbJumpToUserDef->setEnabled(false);
		tbRun->setEnabled(false);
		tbPause->setEnabled(true);
		tbToggleGuiUpdate->setEnabled(true);
		tbToggleNoTrace->setEnabled(false);
		tbToggleHaltOnError->setEnabled(false);
		tbSave->setEnabled(false);
		tbGlTraceSettings->setEnabled(false);
		setGuiUpdates(true);
		break;
	case RL_DBG_VERTEX_SHADER:
	case RL_DBG_GEOMETRY_SHADER:
	case RL_DBG_FRAGMENT_SHADER:
		currentRunLevel = rl;
		title.append(" - ");
		title.append(dbgProgArgs[0]);
		this->setWindowTitle(title);
		aOpen->setEnabled(true);
#ifdef _WIN32
		//aAttach->setEnabled(true);
		// TODO
#endif /* _WIN32 */
		if (!tbBVCaptureAutomatic->isChecked()) {
			tbBVCapture->setEnabled(true);
		}
		tbBVCaptureAutomatic->setEnabled(true);
		tbExecute->setEnabled(true);
		tbExecute->setIcon(
				QIcon(
						QString::fromUtf8(
								":/icons/icons/face-devil-green-grin_32.png")));
		tbStep->setEnabled(false);
		tbSkip->setEnabled(false);
		tbEdit->setEnabled(false);
		tbJumpToDrawCall->setEnabled(false);
		tbJumpToShader->setEnabled(false);
		tbJumpToUserDef->setEnabled(false);
		tbRun->setEnabled(false);
		tbPause->setEnabled(false);
		tbToggleGuiUpdate->setEnabled(false);
		tbToggleNoTrace->setEnabled(false);
		tbToggleHaltOnError->setEnabled(false);
		tbGlTraceSettings->setEnabled(false);
		tbSave->setEnabled(true);
		setGuiUpdates(true);
		break;
	case RL_DBG_RESTART:
		currentRunLevel = rl;
		title.append(" - ");
		title.append(dbgProgArgs[0]);
		this->setWindowTitle(title);
		aOpen->setEnabled(true);
#ifdef _WIN32
		//aAttach->setEnabled(true);
		// TODO
#endif /* _WIN32 */
		if (!tbBVCaptureAutomatic->isChecked()) {
			tbBVCapture->setEnabled(true);
		}
		tbBVCaptureAutomatic->setEnabled(true);
		tbExecute->setEnabled(true);
		tbExecute->setIcon(
				QIcon(
						QString::fromUtf8(
								":/icons/icons/face-devil-green-grin_32.png")));
		tbStep->setEnabled(true);
		tbSkip->setEnabled(true);
		if (m_pCurrentCall && m_pCurrentCall->isEditable()) {
			tbEdit->setEnabled(true);
		} else {
			tbEdit->setEnabled(false);
		}
		tbJumpToDrawCall->setEnabled(true);
		tbJumpToShader->setEnabled(true);
		tbJumpToUserDef->setEnabled(true);
		tbRun->setEnabled(true);
		tbPause->setEnabled(false);
		tbToggleGuiUpdate->setEnabled(true);
		tbToggleNoTrace->setEnabled(true);
		tbToggleHaltOnError->setEnabled(true);
		tbGlTraceSettings->setEnabled(true);
		tbSave->setEnabled(true);
		setGuiUpdates(true);
		break;
	}
}

void MainWindow::setRunLevelDebuggable(int &primitive, bool has_shaders)
{
	/* choose sub-level */
	if (m_pCurrentCall && m_pCurrentCall->isDebuggable(&primitive) && has_shaders)
		setRunLevel(RL_TRACE_EXECUTE_IS_DEBUGABLE);
	else
		setRunLevel(RL_TRACE_EXECUTE_NO_DEBUGABLE);
}

void MainWindow::setErrorStatus(int status)
{
	QPalette palette;
	int color = Qt::blue;
	QString icon = QString::fromUtf8(":/icons/icons/dialog-error-blue_32.png");
	bool added = false;

	pcErrorCode error = static_cast<pcErrorCode>(status);
	switch (error) {
	/* no error */
	case PCE_NONE:
		lSBErrorIcon->setVisible(false);
		lSBErrorText->setVisible(false);
		statusbar->showMessage("");
		break;
		/* gl errors and other non-critical errors */
	case PCE_GL_INVALID_ENUM:
	case PCE_GL_INVALID_VALUE:
	case PCE_GL_INVALID_OPERATION:
	case PCE_GL_STACK_OVERFLOW:
	case PCE_GL_STACK_UNDERFLOW:
	case PCE_GL_OUT_OF_MEMORY:
	case PCE_GL_TABLE_TOO_LARGE:
	case PCE_DBG_READBACK_NOT_ALLOWED:
		icon = QString::fromUtf8(":/icons/icons/dialog-error-green.png");
		color = Qt::green;
		/* all other errors are considered critical errors */
		addGlTraceWarningItem(getErrorDescription(error));
		added = true;
		/* no break */
	default:
		lSBErrorIcon->setVisible(true);
		lSBErrorIcon->setPixmap(QPixmap(icon));
		lSBErrorText->setVisible(true);
		palette = lSBErrorText->palette();
		palette.setColor(QPalette::WindowText, color);
		lSBErrorText->setPalette(palette);
		lSBErrorText->setText(getErrorInfo(error));
		statusbar->showMessage(getErrorDescription(error));
		// TODO: This shit needs refactoring. I do not want to do it right now.
		if (!added)
			addGlTraceErrorItem(getErrorDescription(error));
		break;
	}
}

void MainWindow::setStatusBarText(QString text)
{
	statusbar->showMessage(text);
}

void MainWindow::setMouseOverValues(int x, int y, const bool *active,
		const QVariant *values)
{
	if (x < 0 || y < 0) {
		lSBCurrentPosition->clear();
		lSBCurrentValueR->clear();
		lSBCurrentValueG->clear();
		lSBCurrentValueB->clear();
	} else {
		lSBCurrentPosition->setText(
				QString::number(x) + "," + QString::number(y));
		if (active[0]) {
			lSBCurrentValueR->setText(values[0].toString());
		} else {
			lSBCurrentValueR->clear();
		}
		if (active[1]) {
			lSBCurrentValueG->setText(values[1].toString());
		} else {
			lSBCurrentValueG->clear();
		}
		if (active[2]) {
			lSBCurrentValueB->setText(values[2].toString());
		} else {
			lSBCurrentValueB->clear();
		}
	}
}

void MainWindow::resetPerFrameStatistics(void)
{
	if (m_pCurrentCall && m_pCurrentCall->isFrameEnd()) {
		m_pGlCallPfst->resetStatistic();
		m_pGlExtPfst->resetStatistic();
		m_pGlxCallPfst->resetStatistic();
		m_pGlxExtPfst->resetStatistic();
		m_pWglCallPfst->resetStatistic();
		m_pWglExtPfst->resetStatistic();
	}
}

void MainWindow::resetAllStatistics(void)
{
	m_pGlCallSt->resetStatistic();
	m_pGlExtSt->resetStatistic();
	m_pGlCallPfst->resetStatistic();
	m_pGlExtPfst->resetStatistic();
	m_pGlxCallSt->resetStatistic();
	m_pGlxExtSt->resetStatistic();
	m_pGlxCallPfst->resetStatistic();
	m_pGlxExtPfst->resetStatistic();
	m_pWglCallSt->resetStatistic();
	m_pWglExtSt->resetStatistic();
	m_pWglCallPfst->resetStatistic();
	m_pWglExtPfst->resetStatistic();
}

bool MainWindow::loadMruProgram(QString& outProgram, QString& outArguments,
		QString& outWorkDir)
{
	QSettings settings;
	outProgram = settings.value("MRU/Program", "").toString();
	outArguments = settings.value("MRU/Arguments", "").toString();
	outWorkDir = settings.value("MRU/WorkDir", "").toString();
	return true;
}

bool MainWindow::saveMruProgram(const QString& program,
		const QString& arguments, const QString& workDir)
{
	QSettings settings;
	settings.setValue("MRU/Program", program);
	settings.setValue("MRU/Arguments", arguments);
	settings.setValue("MRU/WorkDir", workDir);
	return true;
}

