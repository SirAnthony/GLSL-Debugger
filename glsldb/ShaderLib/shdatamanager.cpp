
#include "shdatamanager.h"
#include "docks/shsourcedock.h"
#include "docks/shwatchdock.h"
#include "docks/shdockwidget.h"
#include "watch/shwindowmanager.h"
#include "watch/watchview.h"
#include "data/vertexBox.h"
#include "data/pixelBox.h"
#include "dialogs/selection.h"
#include "dialogs/loop.h"
#include "debuglib.h"
#include "utils/dbgprint.h"
#include "errorCodes.h"
#include "progControl.qt.h"
#include "runLevel.h"
#include "ShaderLang.h"


#include <QMessageBox>
#include <QMainWindow>


ShDataManager* ShDataManager::instance = NULL;

const char *dbg_cg_names[DBG_CG_LAST] = {
	"DBG_CG_ORIGINAL_SRC", "DBG_CG_COVERAGE", "DBG_CG_GEOMETRY_MAP",
	"DBG_CG_GEOMETRY_CHANGEABLE", "DBG_CG_VERTEX_COUNT",
	"DBG_CG_SELECTION_CONDITIONAL", "DBG_CG_SWITCH_CONDITIONAL",
	"DBG_CG_LOOP_CONDITIONAL", "DBG_CG_CHANGEABLE"
};

static const DBG_TARGETS dbg_pc_targets[smCount] = {
	DBG_TARGET_VERTEX_SHADER,
	DBG_TARGET_GEOMETRY_SHADER,
	DBG_TARGET_FRAGMENT_SHADER
};

static const EShLanguage dbg_languages[smCount] = {
	EShLangVertex, EShLangGeometry, EShLangFragment,
};

static const runLevel dbg_runlevels[smCount] = {
	RL_DBG_VERTEX_SHADER,
	RL_DBG_GEOMETRY_SHADER,
	RL_DBG_FRAGMENT_SHADER
};

struct ShadersStackHolder
{
	int count;
	char **shaders;

	ShadersStackHolder(int c)
	{
		count = c;
		shaders = (char **)malloc(count * sizeof(char *));
		for (int i = 0; i < count; ++i)
			shaders[i] = NULL;
	}

	~ShadersStackHolder()
	{
		for (int i=0; i < count; ++i) {
			if (shaders[i])
				free(shaders[i]);
		}
		free(shaders);
	}

	void clear(int num)
	{
		if (count < num || !shaders[num])
			return;
		free(shaders[num]);
		shaders[num] = NULL;
	}
};

static void printDebugInfo(int option, int target, const char * const shaders[3])
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


ShDataManager::ShDataManager(ProgramControl *_pc, QWidget *parent) :
	QObject(parent), shaderMode(smNoop), ready(false), shadersAvaliable(false),
	selectedPixel {-1, -1}, geometry { GL_NONE, GL_POINTS, GL_POINTS, NULL, NULL },
	compiler(NULL), shVariables(NULL), pc(_pc), coverage(NULL)
{
	model = new ShVarModel();
	connect(this, SIGNAL(cleanModel()), model, SLOT(clear()));
	connect(model, SIGNAL(addWatchItem(ShVarItem*)),
			this, SLOT(updateWatched(ShVarItem*)));

	windows = new ShWindowManager(parent);
	ShInitialize();
}

ShDataManager::~ShDataManager()
{
	ShFinalize();
	delete windows;
	delete model;
}

ShDataManager *ShDataManager::create(ProgramControl *pc, QWidget *parent)
{
	if (instance)
		delete instance;
	instance = new ShDataManager(pc, parent);
	return instance;
}

ShDataManager *ShDataManager::get()
{
	Q_ASSERT_X(instance, "ShDataManager", "Manager was not initialized");
	return instance;
}

static DataBox *create_databox(ShaderMode mode)
{
	switch (mode) {
	case smFragment:
		return new PixelBox;
	case smVertex:
	case smGeometry:
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

bool ShDataManager::getDebugData(ShaderMode type, int option, ShChangeableList *cl,
								 int format, bool *coverage, DataBox *data)
{
	if (type < 0) {
		dbgPrint(DBGLVL_ERROR, "ShDataManager::getDebugData called with error type.\n");
		return false;
	}

	pcErrorCode error;
	ShadersStackHolder sh(smCount);
	char *debugCode = ShDebugGetProg(compiler, cl, shVariables, (DbgCgOptions)option);

	emit getShaders(sh.shaders, sh.count);
	sh.clear(type);
	sh.shaders[type] = debugCode;
	if (type == smVertex)
		sh.clear(smGeometry);

	DBG_TARGETS target = dbg_pc_targets[type];
	printDebugInfo(option, target, sh.shaders);
	if (type == smFragment)
		error = static_cast<pcErrorCode>(retriveFragmentData(sh.shaders, format,
						option, coverage, dynamic_cast<PixelBox*>(data)));
	else
		error = static_cast<pcErrorCode>(retriveVertexData(sh.shaders, target,
						option, coverage, dynamic_cast<VertexBox*>(data)));

	if (!processError(error, type))
		return false;

	dbgPrint(DBGLVL_INFO, "getDebugVertexData done\n");
	return true;
}

bool ShDataManager::cleanShader(ShaderMode type)
{
	pcErrorCode error = PCE_NONE;
	shaderMode = smNoop;

	emit cleanDocks(type);
	selectedPixel[0] = selectedPixel[1] = -1;
	if (coverage) {
		delete[] coverage;
		coverage = NULL;
	}

	switch (type) {
	case smGeometry:
	case smVertex:
	case smFragment:
		emit cleanModel();
		while (loopsData.isEmpty())
			delete loopsData.pop();

		if (compiler) {
			ShDestruct(compiler);
			// It is the shader memory, so it was freed just now.
			shVariables = NULL;
			compiler = NULL;
		}

		dbgPrint(DBGLVL_INFO, "Restoring render target\n");
		/* restore render target */
		error = pc->restoreRenderTarget(dbg_pc_targets[type]);
		break;
	default:
		break;
	}

	return processError(error, type);
}

void ShDataManager::getPixels(int p[2])
{
	p[0] = selectedPixel[0];
	p[1] = selectedPixel[1];
}

bool ShDataManager::hasActiveWindow()
{
	return windows->activeWindow() != NULL;
}

const GeometryInfo &ShDataManager::getGeometryInfo() const
{
	return geometry;
}

ShaderMode ShDataManager::getMode()
{
	return shaderMode;
}

int ShDataManager::getCurrentTarget()
{
	int index = -1;
	emit getCurrentIndex(index);

	if (index >= 0 || index < smCount)
		return dbg_pc_targets[index];
	return -1;
}

bool ShDataManager::codeReady()
{
	return ready && shadersAvaliable;
}

ShBuiltInResource *ShDataManager::getResource()
{
	return &shResources;
}

void ShDataManager::execute(ShaderMode type)
{
	if (shaderMode > 0) {
		/* clean up debug run */
		cleanShader(shaderMode);
		emit setRunLevel(RL_DBG_RESTART);
		return;
	}

	int error = PCE_NONE;
	emit saveQueries(error);
	if (!processError(error, type))
		return;

	/* setup debug render target */
	switch (type) {
	case smVertex:
	case smGeometry:
		error = pc->setDbgTarget(dbg_pc_targets[type], DBG_PFT_KEEP,
								 DBG_PFT_KEEP, DBG_PFT_KEEP, DBG_PFT_KEEP);
		break;
	case smFragment: {
		FragmentTestOptions opts;
		emit getOptions(&opts);
		error = pc->setDbgTarget(dbg_pc_targets[type],
								 opts.alphaTest, opts.depthTest,
								 opts.stencilTest, opts.blending);
		break;
	}
	default:
		dbgPrint(DBGLVL_ERROR, "Wrong shader type to execute\n");
		error = PCE_DBG_INVALID_VALUE;
		return;
	}

	if (!processError(error, type, true))
		return;

	emit recordDrawCall(error);
	if (!processError(error, type))
		return;

	ShadersStackHolder sh(smCount);
	emit getShaders(sh.shaders, sh.count);
	if (!sh.shaders[type])
		return;

	emit setRunLevel(dbg_runlevels[type]);
	shaderMode = type;

	/* start building the parse tree for this shader */
	int dbgopts = EDebugOpIntermediate;
	compiler = ShConstructCompiler(dbg_languages[type], dbgopts);
	if (!compiler) {
		processError(PCE_UNKNOWN_ERROR, type);
		return;
	}

	if (!ShCompile(compiler, &sh.shaders[type], 1, &shResources, dbgopts, &shVariables)) {
		const char *err = ShGetInfoLog(compiler);
		QMessageBox message;
		message.setText("Error at shader compilation");
		message.setDetailedText(err);
		message.setIcon(QMessageBox::Critical);
		message.setStandardButtons(QMessageBox::Ok);
		message.exec();
		cleanShader(type);
		emit setRunLevel(RL_DBG_RESTART);
		return;
	}

	/* update model */
	model->clear();
	model->appendRow(shVariables);

	step(DBG_BH_JUMP_INTO);
	updateGeometry(type);
}

void ShDataManager::cleanShader()
{
	cleanShader(shaderMode);
}

void ShDataManager::reset()
{
	step(DBG_BH_RESET, true);
	step(DBG_BH_JUMP_INTO);
	emit updateStepControls(true);
	emit resetWatchData(shaderMode, coverage);
}

void ShDataManager::step(int action, bool update_watch, bool update_covermap)
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
		dbgPrint(DBGLVL_ERROR, "An error occured at shader step.\n");
		return;
	}

	if (dr->status == DBG_RS_STATUS_OK) {
		/* Update scope list and mark changed variables */
		model->setChangedAndScope(dr->cgbls, dr->scope, dr->scopeStack);

		if (update_covermap) {
			DataBox* coverageBox = create_databox(shaderMode);

			/* Retrieve cover map (one render pass 'DBG_CG_COVERAGE') */
			if (!getDebugData(shaderMode, DBG_CG_COVERAGE, NULL,
							  GL_FLOAT, NULL, coverageBox)) {
				dbgPrint(DBGLVL_ERROR, "An error occurred while reading coverage.\n");
				if (coverageBox)
					delete coverageBox;
				cleanShader(shaderMode);
				emit setRunLevel(RL_DBG_RESTART);
				return;
			}

			bool changed;
			int active = coverageBox->getCoverageFromData(&coverage, coverage, &changed);
			emit updateWatchCoverage(shaderMode, coverage);

			if (shaderMode == smFragment) {
				if (active == lastActive)
					cmstatus = COVERAGEMAP_UNCHANGED;
				else if (active > lastActive)
					cmstatus = COVERAGEMAP_GROWN;
				else
					cmstatus = COVERAGEMAP_SHRINKED;
				lastActive = active;
			} else if (shaderMode == smGeometry || shaderMode == smVertex) {
				if (changed) {
					dbgPrint(DBGLVL_INFO, "cmstatus = COVERAGEMAP_GROWN\n");
					cmstatus = COVERAGEMAP_GROWN;
				} else {
					dbgPrint(DBGLVL_INFO, "cmstatus = COVERAGEMAP_UNCHANGED\n");
					cmstatus = COVERAGEMAP_UNCHANGED;
				}
			}

			if (coverageBox)
				delete coverageBox;
		}
	} else if (dr->status == DBG_RS_STATUS_FINISHED) {
		emit updateStepControls(false);
	} else {
		dbgPrint(DBGLVL_ERROR, "An unhandled debug result (%d) occurred.\n", dr->status);
		return;
	}

	if (dr->position == DBG_RS_POSITION_DUMMY)
		emit updateStepControls(false);


	/* Process watch list */
	if (update_watch) {
		dbgPrint(DBGLVL_INFO, "updateWatchData %d emitVertex: %d, discard: %d\n",
				 cmstatus, dr->passedEmitVertex, dr->passedDiscard);
		emit updateWatchData(shaderMode, cmstatus, coverage,
						dr->passedEmitVertex || dr->passedDiscard);
	}

	updateDialogs(dr->position, dr->loopIteration, update_covermap, updateGUI);

	if (updateGUI)
		emit updateSourceHighlight(shaderMode, &dr->range);
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
	ShaderMode mode;
	if (type == ShWindowManager::wtVertex) {
		mode = smVertex;
		text = "Vertex " + QString::number(x);
	} else if (type == ShWindowManager::wtGeometry) {
		mode = smGeometry;
		text = "Primitive " + QString::number(x);
	} else if (type == ShWindowManager::wtFragment) {
		mode = smFragment;
		text = "Pixel " + QString::number(x) + ", " + QString::number(y);
	}

	emit updateSelection(x, y, text, mode);
}

void ShDataManager::updateShaders(int &error)
{
	/* call debug function that reads back the shader code */
	int uniformsCount = 0;
	char *uniforms = NULL;
	char *shaders[smCount];

	error = pc->getShaderCode(shaders, &shResources, &uniforms, &uniformsCount);
	if (error == PCE_NONE) {
		/* show shader code(s) in tabs */
		const char *cshaders[smCount] = { shaders[0], shaders[1], shaders[2] };
		emit setShaders(cshaders, smCount);
		model->setUniforms(uniforms, uniformsCount);
		shadersAvaliable = (shaders[0] || shaders[1] || shaders[2]);
	}

	for (int i = 0; i < smCount; ++i)
		if (shaders[i])
			delete[] shaders[i];
	if (uniforms)
		delete[] uniforms;
}

void ShDataManager::removeShaders()
{
	emit setShaders(NULL, smCount);
	shadersAvaliable = false;
}

void ShDataManager::updateGui(int rl)
{
	int shader_level = -1;

	switch (rl) {
	case RL_INIT:
	case RL_SETUP:
		emit removeShaders();
		break;
	case RL_TRACE_EXECUTE_NO_DEBUGABLE:
	case RL_TRACE_EXECUTE_RUN:
	case RL_DBG_RECORD_DRAWCALL:
		break;
	case RL_DBG_VERTEX_SHADER:
	case RL_DBG_GEOMETRY_SHADER:
	case RL_DBG_FRAGMENT_SHADER:
		shader_level = rl - RL_DBG_VERTEX_SHADER;
		break;
	case RL_TRACE_EXECUTE_IS_DEBUGABLE:
	case RL_DBG_RESTART:
		break;
	default:
		return;
	}

	ready = (rl == RL_DBG_RESTART || rl == RL_TRACE_EXECUTE_IS_DEBUGABLE);
	emit updateControls(shader_level, rl == RL_DBG_RESTART,
						rl == RL_TRACE_EXECUTE_IS_DEBUGABLE);
	emit updateWatchGui(false);
}

void ShDataManager::updateWatched(ShVarItem *item)
{
	item->updateWatchData(shaderMode, coverage);
}

void ShDataManager::updateGeometry(ShaderMode type)
{
	if(type != smGeometry)
		return;

	geometry.inputType = shResources.geoInputType;
	geometry.outputType = shResources.geoOutputType;

	if (geometry.map)
		delete geometry.map;
	if (geometry.count)
		delete geometry.count;
	geometry.map = new VertexBox();
	geometry.count = new VertexBox();
	dbgPrint(DBGLVL_INFO, "Get GEOMETRY_MAP & VERTEX_COUNT:");
	if (getDebugData(type, DBG_CG_GEOMETRY_MAP, NULL, 0, NULL, geometry.map) &&
			getDebugData(type, DBG_CG_VERTEX_COUNT, NULL, 0, NULL, geometry.count)) {
		/* TODO: build geometry model */
	} else {
		cleanShader(type);
		emit setRunLevel(RL_DBG_RESTART);
	}
}

void ShDataManager::updateDialogs(int position, int loop_iter,
								  bool update_covermap, bool update_gui)
{
	DataBox* dataBox = NULL;
	DbgCgOptions options = position_options(static_cast<DbgRsTargetPosition>(position));

	if (options == DBG_CG_ORIGINAL_SRC)
		return;

	bool retrive = true;
	if (position == DBG_RS_POSITION_LOOP_CHOOSE && shaderMode == smFragment)
		retrive = update_covermap;

	if (retrive) {
		dataBox = create_databox(shaderMode);
		if (!getDebugData(shaderMode, options, NULL, GL_FLOAT, coverage, dataBox)) {
			dbgPrint(DBGLVL_WARNING, "An error occurred while retrieving the data.\n");
			cleanShader(shaderMode);
			emit setRunLevel(RL_DBG_RESTART);
			delete dataBox;
			return;
		}
	}

	QSet<ShVarItem *> watchItems;
	/* Create list of all watch item boxes */
	if (shaderMode != smFragment)
		emit getWatchItems(watchItems);

	/* Process position dependent requests */
	switch (position) {
	case DBG_RS_POSITION_SELECTION_IF_CHOOSE:
	case DBG_RS_POSITION_SELECTION_IF_ELSE_CHOOSE: {
		bool ifelse = position == DBG_RS_POSITION_SELECTION_IF_ELSE_CHOOSE;
		SelectionDialog dialog(shaderMode, dataBox, watchItems, geometry, ifelse, windows);
		switch (dialog.exec()) {
		case SelectionDialog::SB_SKIP:
			step(DBG_BH_JUMP_OVER);
			break;
		case SelectionDialog::SB_IF:
			step(DBG_BH_JUMP_INTO);
			break;
		case SelectionDialog::SB_ELSE:
			step(DBG_BH_FOLLOW_ELSE);
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
			dbgPrint(DBGLVL_INFO, "==> new loop encountered\n");
			lData = new LoopData(shaderMode, dataBox, this);
			loopsData.push(lData);
		} else {
			dbgPrint(DBGLVL_INFO, "==> known loop at %d\n", loop_iter);
			if (!loopsData.isEmpty()) {
				lData = loopsData.top();
				if (update_covermap)
					lData->addLoopIteration(shaderMode, dataBox, loop_iter);
			} else {
				/* TODO error handling */
				dbgPrint(DBGLVL_ERROR, "An error occurred while trying to "
						 "get loop count data.\n");
				step(DBG_BH_JUMP_OVER);
				delete dataBox;
				return;
			}
		}
		delete dataBox;

		if (update_gui) {
			LoopDialog lDialog(shaderMode, lData, watchItems, geometry, windows);
			connect(&lDialog, SIGNAL(doStep(int, bool, bool)), this, SLOT(step(int,bool,bool)));

			switch (lDialog.exec()) {
			case LoopDialog::SA_NEXT:
				step(DBG_BH_LOOP_NEXT_ITER);
				break;
			case LoopDialog::SA_BREAK:
				step (DBG_BH_JUMP_OVER);
				break;
			case LoopDialog::SA_JUMP:
				/* Force update of all changed items */
				emit updateWatchData(shaderMode, COVERAGEMAP_GROWN, coverage, false);
				step(DBG_BH_JUMP_INTO);
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

bool ShDataManager::processError(int error, ShaderMode type, bool restart)
{
	pcErrorCode code = static_cast<pcErrorCode>(error);
	if (code == PCE_RETURN)
		return false;

	emit setErrorStatus(error);
	if (code == PCE_NONE)
		return true;

	dbgPrint(DBGLVL_WARNING, "Error occurred: %s\n", getErrorDescription(code));
	if (isErrorCritical(code)) {
		dbgPrint(DBGLVL_ERROR, "Error is critical.\n");
		cleanShader(type);
		emit setRunLevel(RL_SETUP);
		emit killProgram(1);
	} else if (restart) {
		dbgPrint(DBGLVL_ERROR, "Restart required.\n");
		emit setRunLevel(RL_DBG_RESTART);
	}
	return false;
}

int ShDataManager::retriveVertexData(const char * const shaders[], int target, int option,
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

	dbgPrint(DBGLVL_DEBUG, "retriveVertexData done\n");
	return error;
}

int ShDataManager::retriveFragmentData(const char * const shaders[], int format, int option,
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

	dbgPrint(DBGLVL_COMPILERINFO, "Init buffers...\n");
	switch (option) {
	case DBG_CG_ORIGINAL_SRC:
		error = pc->initializeRenderBuffer(true, true, true, true, 0.0, 0.0,
				0.0, 0.0, 0.0, 0);
		break;
	case DBG_CG_COVERAGE:
	case DBG_CG_SELECTION_CONDITIONAL:
	case DBG_CG_SWITCH_CONDITIONAL:
	case DBG_CG_LOOP_CONDITIONAL:
	case DBG_CG_CHANGEABLE: {
		FragmentTestOptions opts;
		emit getOptions(&opts);
		error = pc->initializeRenderBuffer(opts.copyRGB, opts.copyAlpha,
				opts.copyDepth, opts.copyStencil, opts.redValue, opts.greenValue,
				opts.blueValue, opts.alphaValue, opts.depthValue, opts.stencilValue);
		break;
	}
	default:
		dbgPrint(DBGLVL_ERROR, "Unhandled DbgCgCoption %i\n", option);
		error = PCE_UNKNOWN_ERROR;
		break;
	}

	if (error != PCE_NONE)
		return error;

	int width, height;
	void *imageData;
	error = pc->shaderStepFragment(shaders, channels, format, &width, &height, &imageData);

	if (error != PCE_NONE)
		return error;

	switch (format) {
	case GL_FLOAT:
		box->setData<float>(width, height, channels, (float *)imageData, coverage);
		break;
	case GL_INT:
		box->setData<int>(width, height, channels, (int *)imageData, coverage);
		break;
	case GL_UNSIGNED_INT:
		box->setData<unsigned int>(width, height, channels, (unsigned *)imageData, coverage);
		break;
	default:
		dbgPrint(DBGLVL_ERROR, "Invalid image data format\n");
	}

	free(imageData);
	dbgPrint(DBGLVL_DEBUG, "retriveFragmentData done\n");
	return error;
}


