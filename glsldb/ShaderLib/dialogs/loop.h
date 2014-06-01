
#ifndef LOOPDIALOG_H
#define LOOPDIALOG_H

#include "loopdata.h"
#include <QList>
#include <QDialog>

class ShVarItem;
class QLabel;
class QSignalMapper;

namespace Ui {
	class ShLoop;
}


class LoopDialog: public QDialog {
Q_OBJECT

public:
	LoopDialog(ShaderMode mode, LoopData *data, QSet<ShVarItem*> &watchItems,
			   GeometryInfo &geometry, QWidget *parent = 0);

	typedef enum {
		SA_NEXT = 0,
		SA_BREAK,
		SA_JUMP
	} selectedAction;

	int exec();

signals:
	void doStep(int, bool, bool);

protected slots:
	void stateChanged(int, int);
	void calculateStatistics();
	void nextIteration();
	void doJump();
	void doBreak();
	void stopProgress();
	void stopJump();

private slots:
	/* auto-connect */
	void on_cbActive_stateChanged(int);
	void on_cbOut_stateChanged(int);
	void on_cbDone_stateChanged(int);

	/* self connect */
	void reorganizeLoopTable(const QModelIndex&, int, int);

protected:
	Ui::ShLoop *ui;
	void setupGui(void);
	void updateIterationStatistics();

private:
	QLabel *image;
	LoopData *loopData;
	int activeValues;
	bool processStopped;
};

#endif /* LOOPDIALOG_H */
