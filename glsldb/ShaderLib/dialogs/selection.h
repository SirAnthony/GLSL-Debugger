
#ifndef SELECTIONDIALOG_H
#define SELECTIONDIALOG_H

#include <QDialog>
#include <QSet>
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
	SelectionDialog(ShaderMode, DataBox *, QSet<ShVarItem*> &,
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
	QWidget *vertexWidget(VertexBox *, QSet<ShVarItem*> &);
	QWidget *geometryWidget(VertexBox *, QSet<ShVarItem*> &, GeometryInfo &);
	QWidget *fragmentWidget(PixelBox *);
	void displayStatistics(void);

protected slots:
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

