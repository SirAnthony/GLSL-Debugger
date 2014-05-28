
#include "shdatamanager.h"
#include "docks/shsourcedock.h"
#include "docks/shwatchdock.h"
#include "docks/shdockwidget.h"
#include "watch/shwindowmanager.h"
#include "watch/watchview.h"
#include "data/vertexBox.h"
#include "data/pixelBox.h"
#include "debuglib.h"
#include "utils/dbgprint.h"
#include "errorCodes.h"
#include "progControl.qt.h"
#include "runLevel.h"


#include <QMessageBox>
#include <QMainWindow>


ShDataManager* ShDataManager::instance = NULL;

const char *dbg_cg_names[DBG_CG_LAST] = {
	"DBG_CG_ORIGINAL_SRC", "DBG_CG_COVERAGE", "DBG_CG_GEOMETRY_MAP",
	"DBG_CG_GEOMETRY_CHANGEABLE", "DBG_CG_VERTEX_COUNT",
	"DBG_CG_SELECTION_CONDITIONAL", "DBG_CG_SWITCH_CONDITIONAL",
	"DBG_CG_LOOP_CONDITIONAL", "DBG_CG_CHANGEABLE"
};

static void printDebugInfo(int option, int target, const char *shaders[3])
{
	/////// DEBUG
	dbgPrint(DBGLVL_DEBUG, ">>>>> DEBUG CG: ");
	dbgPrintNoPrefix(DBGLVL_COMPILERINFO, "%s\n", dbg_cg_names[option]);
	if (target == DBG_TARGET_VERTEX_SHADER)
		dbgPrint(DBGLVL_COMPILERINFO, ">>>>> DEBUG VERTEX SHADER:\n %s\n", shaders[0]);
	else if (target == DBG_TARGET_GEOMETRY_SHADER)
		dbgPrint(DBGLVL_COMPILERINFO, ">>>>> DEBUG GEOMETRY SHADER:\n %s\n", shaders[1]);
	else
		dbgPrint(DBGLVL_COMPILERINFO, ">>>>> DEBUG FRAGMENT SHADER:\n %s\n", shaders[2]);
}


ShDataManager::ShDataManager(QMainWindow *window, ProgramControl *_pc, QObject *parent) :
	QObject(parent), selectedPixel {-1, -1},
	shVariables(NULL), compiler(NULL), pc(_pc), coverage(NULL)
{
	geometry.primitiveMode = GL_NONE;
	geometry.outputType = resources.geoOutputType;
	geometry.map = NULL;
	geometry.count = NULL;

	model = new ShVarModel();
	windows = new ShWindowManager(window);
	window->setCentralWidget(windows);

}

ShDataManager *ShDataManager::create(QMainWindow *window, ProgramControl *pc, QObject *parent)
{
	if (instance)
		delete instance;
	instance = new ShDataManager(window, pc, parent);
	return instance;
}

ShDataManager *ShDataManager::get()
{
	Q_ASSERT_X(instance, "ShDataManager", "Manager was not initialized");
	return instance;
}


void ShDataManager::registerDock(ShDockWidget *dock, DockType type)
{
	docks[type] = dock;
	dock->setModel(model);

	connect(this, SIGNAL(cleanDocks(EShLanguage)), dock, SLOT(cleanDock(EShLanguage)));
	connect(dock, SIGNAL(updateWindows(bool)), windows, SLOT(updateWindows(bool)));	

	if (type == dmDTWatch) {
		ShWatchDock* wdock = dynamic_cast<ShWatchDock *>(dock);
		connect(wdock, SIGNAL(createWindow(QList<ShVarItem*>&,int)),
				windows, SLOT(createWindow(QList<ShVarItem*>&,ShWindowManager::WindowType)));
		connect(wdock, SIGNAL(extendWindow(QList<ShVarItem*>,int)),
				windows, SLOT(extendWindow(QList<ShVarItem*>,ShWindowManager::WindowType)));
		connect(this, SIGNAL(updateSelection(int,int,QString&,EShLanguage)),
				wdock, SLOT(updateSelection(int,int,QString&,EShLanguage)));
	} else if (type == dmDTSource) {
		ShSourceDock* sdock = dynamic_cast<ShSourceDock *>(dock);
		connect(this, SIGNAL(updateStepControls(bool)), sdock, SLOT(updateControls(bool)));
		connect(this, SIGNAL(updateSourceHighlight(EShLanguage,DbgRsRange&)),
				sdock, SLOT(updateHighlight(EShLanguage,DbgRsRange&)));
	}
}

static DataBox *create_databox(EShLanguage type)
{
	switch (mode) {
	case EShLangFragment:
		return new PixelBox;
	case EShLangGeometry:
	case EShLangVertex:
		return new VertexBox;
	default:
		return NULL;
	}
}

static DbgCgOptions position_options(DbgRsTargetPosition position)
{
	switch (position) {
	case DBG_RS_POSITION_SELECTION_IF_CHOOSE:
	case DBG_RS_POSITION_SELECTION_IF_ELSE_CHOOSE:
		return DBG_CG_SELECTION_CONDITIONAL;
	case DBG_RS_POSITION_SWITCH_CHOOSE:
		return DBG_CG_SWITCH_CONDITIONAL;
	case DBG_RS_POSITION_LOOP_CHOOSE:
		return DBG_CG_LOOP_CONDITIONAL;
	default:
		return DBG_CG_ORIGINAL_SRC;
	}
}


void ShDataManager::step(int action, bool updateWatchData, bool updateCovermap)
{
	bool updateGUI = true;
	switch (action) {
	case DBG_BH_RESET:
	case DBG_BH_JUMP_INTO:
	case DBG_BH_FOLLOW_ELSE:
	case DBG_BH_JUMP_OVER:
		updateGUI = true;
		break;
	case DBG_BH_LOOP_NEXT_ITER:
		updateGUI = false;
		break;
	}

	DbgResult *dr = NULL;
	int debugOptions = EDebugOpIntermediate;
	CoverageMapStatus cmstatus = COVERAGEMAP_UNCHANGED;
	dr = ShDebugJumpToNext(compiler, debugOptions, action);

	// Fuck this
	static int lastActive = 0;

	if (!dr) {
		dbgPrint(DBGLVL_ERROR, "An error occured at shader step.");
		return;
	}

	if (dr->status == DBG_RS_STATUS_OK) {
		/* Update scope list and mark changed variables */
		model->setChangedAndScope(dr->cgbls, dr->scope, dr->scopeStack);

		if (updateCovermap) {
			DataBox* coverageBox = create_databox(shaderMode);

			/* Retrieve cover map (one render pass 'DBG_CG_COVERAGE') */
			if (!getDebugData(shaderMode, DBG_CG_COVERAGE, NULL,
							  GL_FLOAT, NULL, coverageBox)) {
				dbgPrint(DBGLVL_ERROR, "An error occurred while reading coverage.");
				if (coverageBox)
					delete coverageBox;
				cleanShader(shaderMode);
				emit setRunLevel(RL_DBG_RESTART);
				return;
			}

			bool changed;
			int active = coverageBox->getCoverageFromData(
									&coverage, coverage, &changed);
			updateWatchItemsCoverage(coverage);

			if (mode == EShLangFragment) {
				if (active == lastActive)
					cmstatus = COVERAGEMAP_UNCHANGED;
				else if (active > lastActive)
					cmstatus = COVERAGEMAP_GROWN;
				else
					cmstatus = COVERAGEMAP_SHRINKED;
				lastActive = active;
			} else if (mode == EShLangGeometry || mode == EShLangVertex) {
				if (changed) {
					dbgPrint(DBGLVL_INFO, "cmstatus = COVERAGEMAP_GROWN");
					cmstatus = COVERAGEMAP_GROWN;
				} else {
					dbgPrint(DBGLVL_INFO, "cmstatus = COVERAGEMAP_UNCHANGED");
					cmstatus = COVERAGEMAP_UNCHANGED;
				}
			}

			if (coverageBox)
				delete coverageBox;
		}
	} else if (dr->status == DBG_RS_STATUS_FINISHED) {
		emit updateStepControls(false);
	} else {
		dbgPrint(DBGLVL_ERROR, "An unhandled debug result (%d) occurred.", dr->status);
		return;
	}

	if (dr->position == DBG_RS_POSITION_DUMMY)
		emit updateStepControls(false);


	/* Process watch list */
	if (updateWatchData) {
		dbgPrint(DBGLVL_INFO, "updateWatchData %d emitVertex: %d, discard: %d",
				 cmstatus, dr->passedEmitVertex, dr->passedDiscard);
		updateWatchListData(cmstatus, dr->passedEmitVertex || dr->passedDiscard);
	}

	updateDialogs(dr->position, dr->loopIteration, updateCovermap, updateGUI);

	if (updateGUI)
		emit updateSourceHighlight(shaderMode, dr->range);
}

bool ShDataManager::getDebugData(EShLanguage type, DbgCgOptions option, ShChangeableList *cl,
								 int format, bool *coverage, DataBox *data)
{
	enum DBG_TARGETS target;
	pcErrorCode error;
	const char *shaders[3];
	char *debugCode = ShDebugGetProg(compiler, cl, shVariables, option);
	static_cast<ShSourceDock*>(docks[dmDTSource])->getSource(shaders);

	switch (type) {
	case EShLangVertex:
		shaders[0] = debugCode;
		shaders[1] = NULL;
		target = DBG_TARGET_VERTEX_SHADER;
		break;
	case EShLangGeometry:
		shaders[1] = debugCode;
		target = DBG_TARGET_GEOMETRY_SHADER;
		break;
	case EShLangFragment:
		shaders[2] = debugCode;
		target = DBG_TARGET_FRAGMENT_SHADER;
	default:
		dbgPrint(DBGLVL_ERROR, "ShDataManager::getDebugData called with error type.");
		free(debugCode);
		return false;
	}

	printDebugInfo(option, target, shaders);
	if (type == EShLangFragment)
		error = static_cast<pcErrorCode>(retriveFragmentData(shaders, format,
						option, coverage, dynamic_cast<PixelBox*>(data)));
	else
		error = static_cast<pcErrorCode>(retriveVertexData(shaders, target,
						option, coverage, dynamic_cast<VertexBox*>(data)));

	free(debugCode);
	if (!processError(error, type))
		return false;

	dbgPrint(DBGLVL_INFO, "getDebugVertexData done");
	return true;
}

bool ShDataManager::cleanShader(EShLanguage type)
{
	pcErrorCode error = PCE_NONE;
	emit cleanDocks(type);

	selectedPixel[0] = selectedPixel[1] = -1;
	if (coverage) {
		delete[] coverage;
		coverage = NULL;
	}

	switch (type) {
	case EShLangGeometry:
		//delete m_pGeoDataModel;
	case EShLangVertex:
	case EShLangFragment:
		emit cleanModel();
		while (!m_qLoopData.isEmpty())
			delete m_qLoopData.pop();

		if (compiler) {
			ShDestruct(compiler);
			// It is the shader memory, so it was freed just now.
			shVariables = NULL;
			compiler = NULL;
		}

		dbgPrint(DBGLVL_INFO, "Restoring render target");
		/* restore render target */
		switch (type) {
		case EShLangVertex:
			error = pc->restoreRenderTarget(DBG_TARGET_VERTEX_SHADER);
			break;
		case EShLangGeometry:
			error = pc->restoreRenderTarget(DBG_TARGET_GEOMETRY_SHADER);
			break;
		case EShLangFragment:
			error = pc->restoreRenderTarget(DBG_TARGET_FRAGMENT_SHADER);
			break;
		default:
			error = PCE_NONE;
		}
		break;
	default:
		break;
	}

	return processError(error, type);
}

void ShDataManager::getPixels(int *p[2])
{
	*p = selectedPixel;
}

bool ShDataManager::hasActiveWindow()
{
	return windows->activeWindow() != NULL;
}

const GeometryInfo &ShDataManager::getGeometryInfo() const
{
	return geometry;
}

EShLanguage ShDataManager::getLang()
{
	return shaderMode;
}

void ShDataManager::selectionChanged(int x, int y)
{
	WatchView *view = dynamic_cast<WatchView *>(sender());
	if (!view)
		return;

	int type = view->getType();
	if (x < 0 || (type == ShWindowManager::wtFragment && y < 0))
		return;

	selectedPixel[0] = x;
	selectedPixel[1] = y;

	QString text("Error");
	EShLanguage mode;
	if (type == ShWindowManager::wtVertex) {
		mode = EShLangVertex;
		text = "Vertex " + QString::number(x);
	} else if (type == ShWindowManager::wtGeometry) {
		mode = EShLangGeometry;
		text = "Primitive " + QString::number(x);
	} else if (type == ShWindowManager::wtFragment) {
		mode = EShLangFragment;
		text = "Pixel " + QString::number(x) + ", " + QString::number(y);
	}

	emit updateSelection(x, y, text, mode);
}

void ShDataManager::updateDialogs(DbgRsTargetPosition position, int loop_iter,
								  bool update_covermap, bool update_gui)
{
	DataBox* dataBox = NULL;
	DbgCgOptions options = position_options(position);

	if (options == DBG_CG_ORIGINAL_SRC)
		return;

	bool retrive = true;
	if (position == DBG_RS_POSITION_LOOP_CHOOSE && shaderMode == EShLangFragment)
		retrive = update_covermap;

	if (retrive) {
		dataBox = create_databox(shaderMode);
		if (!getDebugData(shaderMode, options, NULL, GL_FLOAT, coverage, dataBox)) {
			dbgPrint(DBGLVL_WARNING, "An error occurred while retrieving the data.");
			cleanShader(shaderMode);
			emit setRunLevel(RL_DBG_RESTART);
			delete dataBox;
			return;
		}
	}

	QList<ShVarItem*> watchItems;
	/* Create list of all watch item boxes */
	if (model && shaderMode != EShLangFragment)
		watchItems = model->getAllWatchItemPointers();


	/* Process position dependent requests */
	switch (position) {
	case DBG_RS_POSITION_SELECTION_IF_CHOOSE:
	case DBG_RS_POSITION_SELECTION_IF_ELSE_CHOOSE: {
		bool ifelse = dr->position == DBG_RS_POSITION_SELECTION_IF_ELSE_CHOOSE;
		SelectionDialog dialog(shaderMode, dataBox, watchItems, geometry, ifelse, this);
		switch (dialog.exec()) {
		case SelectionDialog::SB_SKIP:
			ShaderStep(DBG_BH_JUMP_OVER);
			break;
		case SelectionDialog::SB_IF:
			ShaderStep(DBG_BH_JUMP_INTO);
			break;
		case SelectionDialog::SB_ELSE:
			ShaderStep(DBG_BH_FOLLOW_ELSE);
			break;
		}
	}
		break;
	case DBG_RS_POSITION_SWITCH_CHOOSE: {
		// TODO: switch choose
	}
		break;
	case DBG_RS_POSITION_LOOP_CHOOSE: {
		LoopData *lData = NULL;
		/* Add data to the loop storage */
		if (loop_iter == 0) {
			dbgPrint(DBGLVL_INFO, "==> new loop encountered");
			lData = new LoopData(dataBox, this);
			m_qLoopData.push(lData);
		} else {
			dbgPrint(DBGLVL_INFO, "==> known loop at %d", loop_iter);
			if (!m_qLoopData.isEmpty()) {
				lData = m_qLoopData.top();
				if (updateCovermap)
					lData->addLoopIteration(dataBox, loop_iter);
			} else {
				/* TODO error handling */
				dbgPrint(DBGLVL_ERROR, "An error occurred while trying to "
						 "get loop count data.");
				ShaderStep(DBG_BH_JUMP_OVER);
				delete dataBox;
				return;
			}
		}
		delete dataBox;

		if (update_gui) {
			LoopDialog lDialog(lData, watchItems, geometry, this);
			connect(&lDialog, SIGNAL(doShaderStep(int, bool, bool)),
					this, SLOT(ShaderStep(int, bool, bool)));

			switch (lDialog.exec()) {
			case LoopDialog::SA_NEXT:
				ShaderStep(DBG_BH_LOOP_NEXT_ITER);
				break;
			case LoopDialog::SA_BREAK:
				ShaderStep (DBG_BH_JUMP_OVER);
				break;
			case LoopDialog::SA_JUMP:
				/* Force update of all changed items */
				updateWatchListData(COVERAGEMAP_GROWN, false);
				ShaderStep(DBG_BH_JUMP_INTO);
				break;
			}
			disconnect(&lDialog, 0, 0, 0);
		}
	}
		break;
	default:
		break;
	}
}

bool ShDataManager::processError(int error, EShLanguage type)
{
	emit setErrorStatus(error);
	if (error == PCE_NONE)
		return true;

	dbgPrint(DBGLVL_WARNING, "Error occured: " << getErrorDescription(error));
	if (isErrorCritical(error)) {
		dbgPrint(DBGLVL_ERROR, "Error is critical.");
		cleanShader(type);
		setRunLevel(RL_SETUP);
		killProgram(1);
	}
	return false;
}

int ShDataManager::retriveVertexData(const char *shaders[], int target, int option,
									 bool *coverage, VertexBox *box)
{
	int elementsPerVertex = 3;
	int forcePointPrimitiveMode = 0;
	pcErrorCode error;

	switch (option) {
	case DBG_CG_GEOMETRY_MAP:
		break;
	case DBG_CG_VERTEX_COUNT:
		forcePointPrimitiveMode = 1;
		break;
	case DBG_CG_GEOMETRY_CHANGEABLE:
		elementsPerVertex = 2;
		break;
	case DBG_CG_CHANGEABLE:
	case DBG_CG_COVERAGE:
	case DBG_CG_SELECTION_CONDITIONAL:
	case DBG_CG_SWITCH_CONDITIONAL:
	case DBG_CG_LOOP_CONDITIONAL:
		elementsPerVertex = 1;
		if (target == DBG_TARGET_GEOMETRY_SHADER)
			forcePointPrimitiveMode = 1;
		break;
	default:
		elementsPerVertex = 1;
		forcePointPrimitiveMode = 0;
		break;
	}

	int numVertices, numPrimitives;
	float *data;
	error = pc->shaderStepVertex(shaders, target, geometry.primitiveMode,
								 forcePointPrimitiveMode, elementsPerVertex, &numPrimitives,
								 &numVertices, &data);
	dbgPrint(DBGLVL_INFO, "retriveVertexData: numPrimitives=%i numVertices=%i\n",
			 numPrimitives, numVertices);

	if (error == PCE_NONE)
		box->setData(data, elementsPerVertex, numVertices, numPrimitives, coverage);

	if (data)
		free(data);

	dbgPrint(DBGLVL_DEBUG, "retriveVertexData done");
	return error;
}

int ShDataManager::retriveFragmentData(const char *shaders[], int format, int option,
									   bool *coverage, PixelBox *box)
{
	int channels;
	pcErrorCode error;

	switch (option) {
	case DBG_CG_CHANGEABLE:
	case DBG_CG_COVERAGE:
	case DBG_CG_SELECTION_CONDITIONAL:
	case DBG_CG_SWITCH_CONDITIONAL:
	case DBG_CG_LOOP_CONDITIONAL:
		channels = 1;
		break;
	default:
		channels = 3;
		break;
	}

	dbgPrint(DBGLVL_COMPILERINFO, "Init buffers...");
	switch (option) {
	case DBG_CG_ORIGINAL_SRC:
		error = pc->initializeRenderBuffer(true, true, true, true, 0.0, 0.0,
				0.0, 0.0, 0.0, 0);
		break;
	case DBG_CG_COVERAGE:
	case DBG_CG_SELECTION_CONDITIONAL:
	case DBG_CG_SWITCH_CONDITIONAL:
	case DBG_CG_LOOP_CONDITIONAL:
	case DBG_CG_CHANGEABLE:
		error = pc->initializeRenderBuffer(false, m_pftDialog->copyAlpha(),
				m_pftDialog->copyDepth(), m_pftDialog->copyStencil(), 0.0, 0.0,
				0.0, m_pftDialog->alphaValue(), m_pftDialog->depthValue(),
				m_pftDialog->stencilValue());
		break;
	default:
		dbgPrint(DBGLVL_ERROR, "Unhandled DbgCgCoption %i", option);
		error = PCE_UNKNOWN_ERROR;
		break;
	}

	if (error != PCE_NONE)
		return error;

	int width, height;
	void *imageData;
	error = pc->shaderStepFragment(shaders, channels, rbFormat, &width, &height, &imageData);

	if (error != PCE_NONE)
		return error;

	switch (format) {
	case GL_FLOAT:
		box->setData<float>(width, height, channels, imageData, coverage);
		break;
	case GL_INT:
		box->setData<int>(width, height, channels, imageData, coverage);
		break;
	case GL_UNSIGNED_INT:
		box->setData<unsigned int>(width, height, channels, imageData, coverage);
		break;
	default:
		dbgPrint(DBGLVL_ERROR, "Invalid image data format");
	}

	free(imageData);
	dbgPrint(DBGLVL_DEBUG, "retriveFragmentData done");
	return error;
}


