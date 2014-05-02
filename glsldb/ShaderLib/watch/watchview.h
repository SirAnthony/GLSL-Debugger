#ifndef WATCHVIEW_H
#define WATCHVIEW_H

#include <QWidget>

class WatchView : public QWidget
{
	Q_OBJECT
public:
	explicit WatchView(QWidget *parent = 0);
	virtual void updateView(bool force) = 0;
	inline void setActive(bool b) {
		active = b;
	}
	
protected:
	bool active;
};

#endif // WATCHVIEW_H
