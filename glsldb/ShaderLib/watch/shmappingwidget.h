
#ifndef SHMAPPINGWIDGET_H
#define SHMAPPINGWIDGET_H

#include <QWidget>
class DataBox;

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
	int currentValIndex();
	
signals:
	void connectDataBox(int);
	void updateScatter();
	void updateView(bool);
	void clearData();
	void updateData(int, int, float, float);
	void getBoundaries(int, double *, double *, bool);
	void getDataBox(int, DataBox **);
	
public slots:
	void cbValActivated(int);
	void cbMapActivated(int);
	void updateData();
	void mappingDataChanged();
	void buttonClicked();
	void switchBoundaries();

protected:
	void updateRangeMapping(int, int);

private:
	Ui::ShMappingWidget *ui;
	
};

#endif // SHMAPPINGWIDGET_H
