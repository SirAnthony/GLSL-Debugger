#ifndef SHDATAMANAGER_H
#define SHDATAMANAGER_H

#include <QObject>
#include <QStack>
#include "ShaderLang.h"

class DataBox;
class VertexBox;
class PixelBox;
class LoopData;
class ProgramControl;
class ShDockWidget;
class ShWindowManager;
class ShVarModel;
class ShVarItem;
class QMainWindow;
class FragmentTestOptions;

struct GeometryInfo {
	int primitiveMode;
	int inputType;
	int outputType;
	VertexBox* map;
	VertexBox* count;
};

enum ShaderMode {
	smNoop = -2,
	smError = -1,
	smVertex,
	smGeometry,
	smFragment,
	smCount
	// Count is not actual count of elements in enum, but count of valid modes
};

enum CoverageMapStatus {
	COVERAGEMAP_UNCHANGED,
	COVERAGEMAP_GROWN,
	COVERAGEMAP_SHRINKED
};

class ShDataManager : public QObject
{
	Q_OBJECT
public:
	/**
	 * Due to current architecture I have no other choise than
	 * make it a singleton. It must have access to ProgramControl
	 * initialized by MainWindow, also it must be accessed by
	 * other things like ShVarItem which have no direct connection
	 * with MainWindow.
	 */
	static ShDataManager* create(QMainWindow *window, ProgramControl *pc,
								 QObject *parent = 0);
	static ShDataManager* get();

	enum DockType {
		dmDTVar,
		dmDTWatch,
		dmDTSource,
		dmDTCount
	};

	void registerDock(ShDockWidget*, DockType);

	bool getDebugData(ShaderMode type, DbgCgOptions option, ShChangeableList *cl,
					  int format, bool *coverage, DataBox *data);
	bool cleanShader(ShaderMode type);
	void getPixels(int [2]);
	bool hasActiveWindow();
	const GeometryInfo &getGeometryInfo() const;
	ShaderMode getMode();
	bool codeReady();
	ShBuiltInResource *getResource();

	bool isAvaliable()
	{ return shadersAvaliable; }

signals:
	void saveQueries(int&);
	void recordDrawCall(int&);
	void cleanDocks(ShaderMode);
	void cleanModel();
	void setErrorStatus(int);
	void setRunLevel(int);
	void setCurrentDebuggable(int&, bool);
	void killProgram(int);
	void updateSelection(int, int, QString &, ShaderMode);
	void updateSourceHighlight(ShaderMode, DbgRsRange *);
	void updateStepControls(bool);
	void updateControls(int, bool, bool);
	void updateWatchGui(bool);
	void updateWatchCoverage(ShaderMode, bool *);
	void updateWatchData(ShaderMode, CoverageMapStatus, bool *, bool);
	void resetWatchData(ShaderMode, bool *);
	void getWatchItems(QSet<ShVarItem *> &);
	void getOptions(FragmentTestOptions *);
	void getShaders(const char *shaders[]);
	void setShaders(char *shaders[]);


public slots:
	void execute(ShaderMode);
	void cleanShader();
	void reset();
	void step(int action, bool update_watch = true, bool update_covermap = true);
	void selectionChanged(int, int);
	void updateShaders(int &error);
	void removeShaders();

protected slots:
	void updateGui(int);
	void updateWatched(ShVarItem *);

protected:
	void updateGeometry(ShaderMode);
	void updateDialogs(int, int, bool, bool);
	bool processError(int error, ShaderMode type, bool restart = false);
	int retriveVertexData(const char *shaders[3], int target, int option, bool *coverage, VertexBox *box);
	int retriveFragmentData(const char *shaders[3], int format, int option, bool *coverage, PixelBox *box);

private:
	ShDataManager(QMainWindow *window, ProgramControl *_pc, QObject *parent = 0);
	~ShDataManager();
	// Probably we do not need stack here
	QStack<LoopData*> loopsData;
	ShVarModel* model;
	ShWindowManager* windows;
	ShaderMode shaderMode;
	bool ready;
	bool shadersAvaliable;
	int selectedPixel[2];
	GeometryInfo geometry;
	ShHandle compiler;
	ShBuiltInResource shResources;
	ShVariableList* shVariables;
	ProgramControl *pc;
	bool *coverage;
	static ShDataManager* instance;
};

#endif // SHDATAMANAGER_H
