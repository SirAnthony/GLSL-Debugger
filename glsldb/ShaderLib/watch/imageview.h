
#ifndef IMAGEVIEW_H
#define IMAGEVIEW_H

#include <QtGui/QLabel>
#include <QtGui/QImage>
#include <QtGui/QWidget>
#include <QtGui/QRubberBand>


class ImageView: public QLabel {
Q_OBJECT

public:
	enum MouseMode {
		MM_NONE,
		MM_ZOOM,
		MM_PICK,
		MM_MINMAX
	};

	ImageView(QWidget *parent = 0);
	void setImage(QImage &image);
	void setMouseMode(int mouseMode);
	QImage getImage(void)
	{ return image; }

signals:
	void mousePosChanged(int x, int y);
	void picked(int x, int y);
	void viewCenterChanged(int x, int y);
	void minMaxAreaChanged(const QRect& minMaxArea);
	void setMappingBounds();
	void updateFocus(int);

protected:
	virtual void keyPressEvent(QKeyEvent *event);
	virtual void keyReleaseEvent(QKeyEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);
	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
	virtual void paintEvent(QPaintEvent *evt);

	void updateImage();
	void setCustomCursor(const char *name);

	void zoomRegion(const QRect &region);
	void setZoomLevel(const float zoomLevel);
	void canvasToImage(int& inOutX, int& inOutY);

protected:
	QRubberBand *minMaxLens;
	QRubberBand *rubberBand;
	QPoint minMaxLensOrigin;
	QPoint rubberBandOrigin;
	int mouseMode;
	float zoomLevel;
	int lastZoomEvent;
	QImage image;
};

#endif /* IMAGEVIEW_H */
