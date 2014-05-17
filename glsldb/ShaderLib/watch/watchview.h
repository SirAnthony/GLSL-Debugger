#ifndef WATCHVIEW_H
#define WATCHVIEW_H

#include <QWidget>

class QAbstractItemModel;
class ShMappingWidget;
class DataBox;


class WatchView : public QWidget
{
	Q_OBJECT
public:
	explicit WatchView(QWidget *parent = 0);
	virtual void updateView(bool force) = 0;
	virtual QAbstractItemModel * model() = 0;
	inline void setActive(bool b) {
		active = b;
	}

public slots:
	virtual void closeView();
	virtual void updateData(int, int, float, float) = 0;
	virtual void clearData() = 0;
	virtual void setBoundaries(int, double *, double *, bool) = 0;
	virtual void setDataBox(int, DataBox **) = 0;

protected:
	virtual void updateGUI();
	virtual void connectWidget(ShMappingWidget *widget);
	static void clearDataArray(float *, int, int, float);

	bool active;
};

#endif // WATCHVIEW_H
