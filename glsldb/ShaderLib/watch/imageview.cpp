/******************************************************************************

Copyright (C) 2006-2009 Institute for Visualization and Interactive Systems
(VIS), Universität Stuttgart.
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.

  * Redistributions in binary form must reproduce the above copyright notice, this
	list of conditions and the following disclaimer in the documentation and/or
	other materials provided with the distribution.

  * Neither the name of the name of VIS, Universität Stuttgart nor the names
	of its contributors may be used to endorse or promote products derived from
	this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*******************************************************************************/

// This file is not so poorly written as others.
// I rewrote less than 90% this time.


#include <cmath>
#include <QtGui/QFrame>
#include <QtGui/QMouseEvent>
#include <QtGui/QBitmap>
#include <QtGui/QPainter>
#include "imageview.h"

ImageView::ImageView(QWidget *parent) :
		QLabel(parent)
{
	setMouseTracking(true);
	minMaxLens = new QRubberBand(QRubberBand::Rectangle, this);
	rubberBand = new QRubberBand(QRubberBand::Rectangle, this);
	zoomLevel = 1.0f;
	setMouseMode(MM_PICK);
}

void ImageView::setImage(QImage &img)
{
	image = img;
	updateImage();
}

void ImageView::setMouseMode(int mode)
{
	mouseMode = mode;
	switch (mouseMode) {
	case MM_ZOOM:
		setCustomCursor(":/cursors/cursors/zoom-in.png");
		break;
	case MM_PICK:
		setCustomCursor(":/cursors/cursors/pick.png");
		break;
	case MM_MINMAX:
		setCustomCursor(":/cursors/cursors/min-max.png");
		break;
	default:
		break;
	}

	if (mouseMode != MM_MINMAX) {
		minMaxLens->setGeometry(QRect());
		minMaxLens->setVisible(false);
		emit minMaxAreaChanged(minMaxLens->geometry());
		repaint(0, 0, -1, -1);
	}
}

void ImageView::keyPressEvent(QKeyEvent *event)
{
	if (mouseMode == MM_ZOOM) {
		int key = event->key();
		if (key == Qt::Key_Control)
			setCustomCursor(":/cursors/cursors/zoom-out.png");
		else if (key == Qt::Key_Shift)
			setCustomCursor(":/cursors/cursors/zoom-reset.png");
	} else {
		event->ignore();
	}
}

void ImageView::keyReleaseEvent(QKeyEvent *event)
{
	if (mouseMode == MM_ZOOM) {
		int key = event->key();
		if ((key == Qt::Key_Control) || (key == Qt::Key_Shift))
			setCustomCursor(":/cursors/cursors/zoom-in.png");
	} else {
		event->ignore();
	}
}

void ImageView::mouseMoveEvent(QMouseEvent *event)
{
	emit updateFocus(Qt::MouseFocusReason);

	if (visibleRegion().contains(event->pos())) {
		int x = event->x();
		int y = event->y();
		canvasToImage(x, y);
		emit mousePosChanged(x, y);
	} else {
		emit mousePosChanged(-1, -1);
	}

	switch (mouseMode) {
	case MM_ZOOM:
		if (event->modifiers() & Qt::ControlModifier)
			rubberBand->hide();
		else if (rubberBand->isVisible())
			rubberBand->setGeometry(
						QRect(rubberBandOrigin, event->pos()).normalized());
		break;
	case MM_MINMAX:
		if ((event->buttons() & Qt::LeftButton) && minMaxLens->isVisible())
			minMaxLens->setGeometry(
						QRect(minMaxLensOrigin, event->pos()).normalized());
		break;
	default:
		break;
	}
}

void ImageView::mousePressEvent(QMouseEvent *event)
{
	switch (mouseMode) {
	case MM_ZOOM:
		rubberBand->setGeometry(0, 0, 0, 0);
		if (event->modifiers() & Qt::ControlModifier) {
			rubberBand->hide();
		} else {
			rubberBandOrigin = event->pos();
			rubberBand->show();
		}
		break;
	case MM_MINMAX:
		minMaxLens->show();
		repaint(0, 0, -1, -1);
		minMaxLens->setGeometry(QRect());
		minMaxLensOrigin = event->pos();
		break;
	case MM_PICK:
		if (visibleRegion().contains(event->pos())) {
			int x = event->x();
			int y = event->y();
			canvasToImage(x, y);
			emit picked(x, y);
		}
		break;
	default:
		break;
	}
}

void ImageView::mouseReleaseEvent(QMouseEvent *event)
{
	switch (mouseMode) {
	case MM_ZOOM:
		rubberBand->hide();
		if (rubberBand->geometry().isNull()) {
			/* Zoom step. */
			int newCenterX, newCenterY;
			int x = event->x();
			int y = event->y();

			if (event->modifiers() & Qt::ControlModifier) {
				setZoomLevel(zoomLevel < 2.0 ? 1.0 : zoomLevel - 1.0);
				if (zoomLevel != 1) {
					newCenterX = x * (zoomLevel - 1.0) / zoomLevel;
					newCenterY = y * (zoomLevel - 1.0) / zoomLevel;
					emit viewCenterChanged(newCenterX, newCenterY);
					if (visibleRegion().contains(event->pos())) {
						canvasToImage(x, y);
						emit mousePosChanged(x, y);
					} else {
						emit mousePosChanged(-1, -1);
					}
				}
			} else if (event->modifiers() == Qt::ShiftModifier) {
				setZoomLevel(1.0f);
			} else {
				setZoomLevel(zoomLevel + 1.0f);
				newCenterX = x * (zoomLevel + 1.0) / zoomLevel;
				newCenterY = y * (this->zoomLevel + 1.0) / zoomLevel;
				emit viewCenterChanged(newCenterX, newCenterY);
				if (visibleRegion().contains(event->pos())) {
					canvasToImage(x, y);
					emit mousePosChanged(x, y);
				} else {
					emit mousePosChanged(-1, -1);
				}
			}

		} else {
			/* Lasso zoom. */
			if (!(event->modifiers() & Qt::ControlModifier))
				zoomRegion(rubberBand->geometry());
		}
		break;
	case MM_MINMAX: {
		QRect selection = minMaxLens->geometry();
		selection.moveLeft(selection.left() / zoomLevel);
		selection.moveTop(selection.top() / zoomLevel);
		selection.setWidth(::ceilf((float)selection.width() / zoomLevel));
		selection.setHeight(::ceilf((float)selection.height() / zoomLevel));
		emit minMaxAreaChanged(selection);
		minMaxLens->setVisible(false);
		if (event->modifiers() & Qt::ControlModifier)
			emit setMappingBounds();
	}
		break;
	default:
		break;
	}
}

void ImageView::paintEvent(QPaintEvent *evt)
{
	QLabel::paintEvent(evt);
	QRect minMaxRect = minMaxLens->geometry();
	if (!minMaxLens->isVisible() && !minMaxRect.isEmpty()) {
		QPainter painter(this);
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setBrush(Qt::NoBrush);
		painter.setPen(QPen(palette().color(QPalette::Highlight), 1.0, Qt::DashLine));
		painter.drawRect(minMaxRect);
	}
}

void ImageView::updateImage()
{
	int width = zoomLevel * image.width();
	int height = zoomLevel * image.height();
	resize(width, height);
	setPixmap(QPixmap::fromImage(image.scaled(width, height)));
}

void ImageView::zoomRegion(const QRect& region)
{
	QRect size = geometry();						// Canvas size.
	QRect target = region.intersected(size);		// Zoom region.
	QRect visible = visibleRegion().boundingRect();	// Viewport.
	QPoint center = target.center();				// Zoom center.

	float sx = (float)visible.width() / target.width();
	float sy = (float)visible.height() / target.height();
	float smin = (sx < sy) ? sx : sy;
	float scale = smin * (float)size.width() / image.width();

	setZoomLevel(scale);
	emit viewCenterChanged(smin * center.x(), smin * center.y());
}

void ImageView::setZoomLevel(const float zl)
{
	zoomLevel = zl;
	updateImage();
}

void ImageView::canvasToImage(int& inOutX, int& inOutY)
{
	inOutX = (float)inOutX / zoomLevel;
	inOutY = (float)inOutY / zoomLevel;
}

void ImageView::setCustomCursor(const char *name)
{
	QPixmap cursor(QString::fromUtf8(name));
	setCursor(QCursor(cursor));
}

