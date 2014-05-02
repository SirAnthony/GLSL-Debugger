#ifndef SHDATAMANAGER_H
#define SHDATAMANAGER_H

#include <QObject>
#include "data/dataBox.h"
#include "ShaderLang.h"

class VertexBox;
class PixelBox;
class ProgramControl;

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
	static ShDataManager* create(ProgramControl *pc, QObject *parent = 0);
	static ShDataManager* get();

	bool getDebugData(EShLanguage type, DbgCgOptions option, ShChangeableList *cl,
					  int format, bool *coverage, DataBox *data);
	bool cleanShader();

signals:
	void cleanModel();
	void setErrorStatus(int);
	void setRunLevel(int);
	void killProgram(int);
	
protected:
	bool processError(int);
	int retriveVertexData(char *shaders[3], int target, int option, bool *coverage, VertexBox *box);
	int retriveFragmentData(char *shaders[3], int format, int option, bool *coverage, PixelBox *box);
	
private:
	ShDataManager(ProgramControl *_pc, QObject *parent = 0);
	int primitiveMode;
	ShVariableList* shVariables;
	ShHandle compiler;
	ProgramControl *pc;
	static ShDataManager* instance;
};

#endif // SHDATAMANAGER_H
