
#ifndef WATCHVECTOR_H
#define WATCHVECTOR_H

#include "watchview.h"

#define MAX_ATTACHMENTS 16
#define WV_WIDGETS_COUNT 3

namespace Ui {
	class ShWatchVector;
}

class PixelBox;
class ImageView;
class QImage;
class QScrollArea;
class ShMappingWidgetFragment;


class WatchVector: public WatchView {
Q_OBJECT

public:
	WatchVector(QWidget *parent = 0);	
	virtual void updateView(bool force);
	virtual QAbstractItemModel *model();
	virtual void attachData(DataBox *, QString &);
	void connectActions(struct MenuActions *);

public slots:
	virtual void updateGUI();
	virtual void updateData(int, int, float, float);
	virtual void clearData() {}
	virtual void setBoundaries(int, double *, double *, bool);
	virtual void setDataBox(int, DataBox **);

	void saveImage();
	void detachData();	

	void setZoomMode();
	void setPickMode();
	void setMinMaxMode();

signals:
	void mouseOverValuesChanged(int x, int y, const bool *active,
			const QVariant *values);

private slots:
	void updateFocus(Qt::FocusReason);
	void setMousePos(int x, int y);
	void newViewCenter(int x, int y);

private:
	Ui::ShWatchVector *ui;

	int freeMappingsCount();
	int getFreeMapping();
	int dataIndex(PixelBox *f);
	void getActiveChannels(PixelBox *channels[3]);
	QImage* drawNewImage(bool useAlpha);

	ShMappingWidgetFragment *widgets[WV_WIDGETS_COUNT];
	PixelBox *vectorData[MAX_ATTACHMENTS];

	int viewCenter[2];
	bool needsUpdate;
	int activeMappings;
};

#endif /* WATCHVECTOR_H */
