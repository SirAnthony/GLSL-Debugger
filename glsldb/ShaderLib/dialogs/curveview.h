
#ifndef CURVEVIEW_H
#define CURVEVIEW_H

#include <QAbstractItemView>
#include <QMap>

class CurveView: public QAbstractItemView {
Q_OBJECT

public:
	CurveView(QWidget *parent = 0);

	QRect visualRect(const QModelIndex &index) const;
	void scrollTo(const QModelIndex &, ScrollHint) {}
	QModelIndex indexAt(const QPoint &point) const;

	void setBase(int base);
	void addMapping(int col, QColor color);
	void delMapping(int col);
protected slots:
	void dataChanged(const QModelIndex &topLeft,
			const QModelIndex &bottomRight);
	void rowsInserted(const QModelIndex &parent, int start, int end);

protected:
	QModelIndex moveCursor(QAbstractItemView::CursorAction cursorAction,
			Qt::KeyboardModifiers modifiers);

	int horizontalOffset() const;
	int verticalOffset() const;

	bool isIndexHidden(const QModelIndex &) const
	{ return false; }

	void setSelection(const QRect&, QItemSelectionModel::SelectionFlags) {}
	QRegion visualRegionForSelection(const QItemSelection &selection) const;

	void paintEvent(QPaintEvent *event);
	void resizeEvent(QResizeEvent *event);
	void scrollContentsBy(int dx, int dy);

private:
	void updateGeometries();

	int margin;
	int tickSize[2];
	int tickSpace[2];
	int labelSize[2];
	int totalSize[2];
	int axisArrow[2];
	int drawAreaMin[2];
	int drawArea[2];
	int baseIndex;
	QMap<int, QColor> mapping;
};

#endif /* CURVEVIEW_H */

