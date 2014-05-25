
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
	virtual void addOption(int, QString &);
	void delOption(int);
	int currentValIndex();
	int currentValData();
	int currentMapData();
	void minMax(float [2]);
	
signals:
	void connectDataBox(int);
	void updateScatter();
	void updateView(bool);
	void clearData();
	void updateData(int, int, float, float);
	void getBoundaries(int, double *, double *, bool);
	void getDataBox(int, DataBox **);
	
public slots:
	virtual void cbValActivated(int);
	void cbMapActivated(int);
	void updateData();
	void mappingDataChanged();
	void buttonClicked();
	void switchBoundaries();
	void updateBoundaries();

protected:
	void updateRangeMapping(int, int);	
	void updateControls(bool);
	virtual void initValButtons();
	virtual void shiftOptions(int);

protected:
	Ui::ShMappingWidget *ui;
	
};

class ShMappingWidgetFragment : public ShMappingWidget {
public:
	explicit ShMappingWidgetFragment(QWidget *parent = 0) : ShMappingWidget(parent) {}
	virtual void addOption(int, QString &);
	QString asText();
protected:
	virtual void initValButtons();
	virtual void shiftOptions(int) {}
};

#endif // SHMAPPINGWIDGET_H
