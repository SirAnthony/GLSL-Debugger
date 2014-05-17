#include "shdatamanager.h"
#include "docks/shsourcedock.h"
#include "watch/shwindowmanager.h"
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
	QObject(parent), primitiveMode(0), selectedPixel {-1, -1},
	shVariables(NULL), compiler(NULL), pc(_pc), coverage(NULL)
{
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
	connect(this, SIGNAL(cleanDocks(EShLanguage)), dock, SLOT(cleanDock(EShLanguage)));
	connect(dock, SIGNAL(updateWindows(bool)), windows, SLOT(updateWindows(bool)));
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
	error = pc->shaderStepVertex(shaders, target, primitiveMode,
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


