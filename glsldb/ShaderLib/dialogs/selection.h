
#ifndef SELECTIONDIALOG_H
#define SELECTIONDIALOG_H

#include <QDialog>
#include <QList>
#include "shdatamanager.h"

class ShVarItem;
class DataBox;
class PixelBox;
class VertexBox;

namespace Ui {
	class ShSelection;
}


class SelectionDialog: public QDialog {
Q_OBJECT

public:
	SelectionDialog(ShaderMode, DataBox *, QList<ShVarItem*> &,
					GeometryInfo &, bool, QWidget *);

	typedef enum {
		SB_IF = 0,
		SB_SKIP,
		SB_ELSE
	} selectedBranch;

	int exec();

protected:
	void checkCounter();
	bool calculateVertex(VertexBox *);
	QWidget *vertexWidget(VertexBox *, QList<ShVarItem*> &);
	QWidget *geometryWidget(VertexBox *, QList<ShVarItem*> &, GeometryInfo &);
	QWidget *fragmentWidget(PixelBox *);
	void displayStatistics(void);

private slots:
	void skipClick();
	void ifClick();
	void elseClick();


protected:
	Ui::ShSelection *ui;

private:
	bool elseBranch;
	int totalValues;
	int activeValues;
	int pathIf;
	int pathElse;
};

#endif /* SELECTIONDIALOG_H */

