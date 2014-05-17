#ifndef SHDATAMANAGER_H
#define SHDATAMANAGER_H

#include <QObject>
#include "docks/shdockwidget.h"
#include "data/dataBox.h"
#include "ShaderLang.h"

class VertexBox;
class PixelBox;
class ProgramControl;
class ShWindowManager;


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
	static ShDataManager* create(QMainWindow *window, ProgramControl *pc, QObject *parent = 0);
	static ShDataManager* get();

	enum DockType {
		dmDTVar,
		dmDTWatch,
		dmDTSource,
		dmDTCount
	};

	void registerDock(ShDockWidget*, DockType);

	bool getDebugData(EShLanguage type, DbgCgOptions option, ShChangeableList *cl,
					  int format, bool *coverage, DataBox *data);
	bool cleanShader(EShLanguage type);
	void getPixels(int (*)[2]);
	bool hasActiveWindow();

signals:
	void cleanDocks(EShLanguage);
	void cleanModel();
	void setErrorStatus(int);
	void setRunLevel(int);
	void killProgram(int);

protected:
	bool processError(int, EShLanguage type);
	int retriveVertexData(const char *shaders[3], int target, int option, bool *coverage, VertexBox *box);
	int retriveFragmentData(const char *shaders[3], int format, int option, bool *coverage, PixelBox *box);

private:
	ShDataManager(QMainWindow *window, ProgramControl *_pc, QObject *parent = 0);
	ShWindowManager* windows;
	int primitiveMode;
	int selectedPixel[2];
	ShVariableList* shVariables;
	ShHandle compiler;
	ShDockWidget* docks[dmDTCount];
	ProgramControl *pc;
	bool *coverage;
	static ShDataManager* instance;
};

#endif // SHDATAMANAGER_H
