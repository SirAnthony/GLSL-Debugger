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
	// FIXME: this value from resources.
	//        It must not be reference in ideal world
	int &outputType;
	VertexBox* map;
	VertexBox* count;
};

enum ShaderMode {
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
	static ShDataManager* create(QMainWindow *window, ProgramControl *pc, int &geoOut,
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

signals:
	void cleanDocks(ShaderMode);
	void cleanModel();
	void setErrorStatus(int);
	void setRunLevel(int);
	void killProgram(int);
	void updateSelection(int, int, QString &, ShaderMode);
	void updateSourceHighlight(ShaderMode, DbgRsRange &);
	void updateStepControls(bool);	
	void updateWatchCoverage(ShaderMode, bool *);
	void updateWatchData(ShaderMode, CoverageMapStatus, bool *, bool);
	void getWatchItems(QSet<ShVarItem *> &);
	void getOptions(FragmentTestOptions *);

public slots:
	void step(int action, bool update_watch = true, bool update_covermap = true);
	void selectionChanged(int, int);

protected:
	void updateDialogs(int, int, bool, bool);
	bool processError(int, ShaderMode type);
	int retriveVertexData(const char *shaders[3], int target, int option, bool *coverage, VertexBox *box);
	int retriveFragmentData(const char *shaders[3], int format, int option, bool *coverage, PixelBox *box);

private:
	ShDataManager(QMainWindow *window, ProgramControl *_pc, int &geoOut, QObject *parent = 0);
	// Probably we do not need stack here
	QStack<LoopData*> loopsData;
	ShVarModel* model;
	ShWindowManager* windows;
	ShaderMode shaderMode;
	int selectedPixel[2];
	GeometryInfo geometry;
	ShVariableList* shVariables;
	ShHandle compiler;
	ShDockWidget* docks[dmDTCount];
	ProgramControl *pc;
	bool *coverage;
	static ShDataManager* instance;
};

#endif // SHDATAMANAGER_H
