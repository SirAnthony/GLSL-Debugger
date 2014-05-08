#ifndef SHMAPPINGWIDGET_H
#define SHMAPPINGWIDGET_H

#include <QWidget>
#include "mappings.h"

namespace Ui {
	class ShMappingWidget;
}

class ShMappingWidget : public QWidget
{
	Q_OBJECT
public:
	explicit ShMappingWidget(QWidget *parent = 0);
	void addOption(int, QString);
	void delOption(int);
	
signals:
	void updateScatter();
	void updateView(bool);
	void clearData();
	void updateData(int, int, float, float);
	
public slots:
	void cbValActivated(int);
	void cbMapActivated(int);
	void updateData();
	void mappingDataChanged();
	void buttonClicked();
	void switchBoundaries();

protected:
	void updateRangeMapping(int);

private:
	Ui::ShMappingWidget *ui;
	
};

#endif // SHMAPPINGWIDGET_H
