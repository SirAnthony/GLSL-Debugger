
#include "curveview.h"
#include "colors.qt.h"
#include <math.h>
#include <QPen>
#include <QPainter>
#include <QPaintEvent>
#include <QScrollBar>


#if defined(_MSC_VER) && (_MSC_VER < 1800)
inline static int round(double x) {
	int y = (int)(x + 0.5);
	while ((double)y < x - 0.5) y++;
	while ((double)y > x + 0.5) y--;
	return y;
}
#endif /* defined(_MSC_VER) && (_MSC_VER < 1800) */


CurveView::CurveView(QWidget *parent) :
		QAbstractItemView(parent), margin(10),
		tickSize { 64, 2 }, tickSpace { 50, 20 },
		labelSize { 75, 16 }, totalSize { 0, 0 },
		axisArrow { 3, 3 }, drawAreaMin { 256, 100 },
		drawArea { 0, 0 }, baseIndex(-1)
{
	horizontalScrollBar()->setRange(0, 0);
	verticalScrollBar()->setRange(0, 0);
	updateGeometries();
}

void CurveView::setBase(int)
{
	baseIndex = 0;
	viewport()->update();
}

void CurveView::addMapping(int col, QColor color)
{
	mapping[col] = color;
	viewport()->update();
}

void CurveView::delMapping(int col)
{
	mapping.remove(col);
	viewport()->update();
}

void CurveView::dataChanged(const QModelIndex &topLeft,
		const QModelIndex &bottomRight)
{
	QAbstractItemView::dataChanged(topLeft, bottomRight);
	viewport()->update();
}

void CurveView::rowsInserted(const QModelIndex &parent, int start, int end)
{
	QAbstractItemView::rowsInserted(parent, start, end);
	viewport()->update();
}

QRect CurveView::visualRect(const QModelIndex &index) const
{
	if (!index.isValid())
		return QRect();

	// Check if we shoud care about this
	int column = index.column();
	if (column != baseIndex && !mapping.contains(column))
		return QRect();
	return viewport()->rect();
}

QModelIndex CurveView::indexAt(const QPoint &) const
{
	return QModelIndex();
}

QModelIndex CurveView::moveCursor(QAbstractItemView::CursorAction ,
		Qt::KeyboardModifiers)
{
	return QModelIndex();
}

int CurveView::horizontalOffset() const
{
	return horizontalScrollBar()->value();
}

int CurveView::verticalOffset() const
{
	return verticalScrollBar()->value();
}

QRegion CurveView::visualRegionForSelection(const QItemSelection &) const
{
	return QRegion();
}

void CurveView::paintEvent(QPaintEvent *event)
{
	if (!mapping.count() || baseIndex < 0)
		return;

	QStyleOptionViewItem option = viewOptions();
	// QStyle::State state = option.state;

	QBrush background = option.palette.base();
	QPen foreground(option.palette.color(QPalette::WindowText));
	QPen textPen(option.palette.color(QPalette::Text));
	QPen highlightedPen(option.palette.color(QPalette::HighlightedText));

	QPainter painter(viewport());
	painter.setRenderHint(QPainter::Antialiasing);
	painter.fillRect(event->rect(), background);
	painter.setPen(foreground);

	QRect dataRect = QRect(margin + labelSize[0], margin,
			drawArea[0], drawArea[1]);

	painter.save();
	painter.translate(dataRect.x() - horizontalScrollBar()->value(),
			dataRect.y() - verticalScrollBar()->value());

	QAbstractItemModel *m = model();
	QModelIndex lastIndex = m->index(m->rowCount() - 1, baseIndex,
			rootIndex());
	int numIterations = m->data(lastIndex).toInt();

	/* x-axis */
	painter.drawLine(0, drawArea[1], drawArea[0] + axisArrow[0],
			drawArea[1]);
	painter.drawLine(drawArea[0], drawArea[1] + axisArrow[1],
			drawArea[0] + axisArrow[0], drawArea[1]);
	painter.drawLine(drawArea[0], drawArea[1] - axisArrow[1],
			drawArea[0] + axisArrow[0], drawArea[1]);

	/* y-axis */
	painter.drawLine(0, -axisArrow[1], 0, drawArea[1]);
	painter.drawLine(-axisArrow[1], 0, 0, -axisArrow[1]);
	painter.drawLine(axisArrow[1], 0, 0, -axisArrow[1]);

	/* max data value */
	int maxValue = 0;
	for (int row = 0; row < m->rowCount(rootIndex()); ++row) {
		QMap<int, QColor>::const_iterator iter = mapping.constBegin();
		while (iter != mapping.constEnd()) {
			QModelIndex index = m->index(row, iter.key(), rootIndex());
			int value = m->data(index).toInt();
			if (value > maxValue) {
				maxValue = value;
			}
			++iter;
		}
	}

	if (maxValue == 0 || numIterations == 0) {
		painter.restore();
		return;
	}

	/* x-ticks */
	int tx = (int) floor(drawArea[0] / (float) tickSpace[0]);
	int iterPerTickX = ceil(numIterations / (float) tx);

	for (int i = 0; i <= numIterations; i += iterPerTickX) {
		int dx = (int) round((drawArea[0] * i) / (float) numIterations);
		painter.drawLine(dx, drawArea[1] - tickSize[1], dx,
				drawArea[1] + tickSize[1]);
		painter.drawText(dx, drawArea[1] + labelSize[1],
				QString::number(i));
	}

	/* y-ticks */
	int ty = (int) floor(drawArea[1] / (float) tickSpace[1]);
	int iterPerTickY = ceil(maxValue / (float) ty);

	for (int i = 0; i <= maxValue; i += iterPerTickY) {
		int dy = (int) round((drawArea[1] * i) / (float) maxValue);
		painter.drawLine(-tickSize[1], drawArea[1] - dy, tickSize[1],
				drawArea[1] - dy);
		painter.drawText(-labelSize[0] - 2 * tickSize[1],
				drawArea[1] - dy - labelSize[1] / 2, labelSize[0],
				labelSize[1], Qt::AlignRight | Qt::AlignVCenter,
				QString::number(i));
	}

	/* curves */
	int xOld = -10;
	int yOld = 0;
	QMap<int, QColor>::const_iterator iter = mapping.constBegin();
	while (iter != mapping.constEnd()) {
		painter.save();
		painter.setPen(iter.value());

		for (int row = 0; row < m->rowCount(rootIndex()); ++row) {
			QModelIndex index = m->index(row, baseIndex, rootIndex());
			int xValue = m->data(index).toInt();

			index = m->index(row, iter.key(), rootIndex());
			int yValue = m->data(index).toInt();

			if (xValue == xOld + 1) {
				painter.drawLine((drawArea[0] * xOld) / (numIterations),
						drawArea[1] - (drawArea[1] * yOld) / maxValue,
						(drawArea[0] * xValue) / (numIterations),
						drawArea[1] - (drawArea[1] * yValue) / maxValue);
			} else {
				painter.drawPoint((drawArea[0] * xValue) / (numIterations),
						drawArea[1] - (drawArea[1] * yValue) / maxValue);
			}
			xOld = xValue;
			yOld = yValue;
		}

		painter.restore();
		++iter;
	}

	painter.restore();
}

void CurveView::resizeEvent(QResizeEvent *)
{
	updateGeometries();
}

void CurveView::scrollContentsBy(int dx, int dy)
{
	viewport()->scroll(dx, dy);
}

void CurveView::updateGeometries()
{
	int boundaries[2] = {
		viewport()->width(),
		viewport()->height()
	};
	QScrollBar *scrollbars[2] = {
		horizontalScrollBar(),
		verticalScrollBar()
	};

	int dmargin = 2 * margin;
	for (int i = 0; i < 2; ++i) {
		int label = labelSize[i] * (i ? 1 : 2);
		// TickSize was allways 1 perviously.
		totalSize[i] = drawAreaMin[i] + label + tickSize[i] + dmargin;
		if (totalSize[i] < boundaries[i]) {
			totalSize[i] = boundaries[i];
			drawArea[i] = totalSize[i] - label - tickSize[i] - dmargin;
		} else {
			drawArea[i] = drawAreaMin[i];
		}

		scrollbars[i]->setPageStep(boundaries[i]);
		scrollbars[i]->setRange(0, qMax(0, totalSize[i] - boundaries[i]));
	}
}

