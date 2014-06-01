
#ifndef DBGCOLORS_H
#define DBGCOLORS_H

#include <QColor>

// TODO: move from here
static inline QString to_string(int base, int total, QString dflt = QString(""))
{
	if (!total)
		return dflt;
	return QString::number(base) + " (" +
			QString::number(base * 100 / total) + "%)";
}

#define DBG_GREEN  (QColor(99, 180, 39))
#define DBG_RED    (QColor(213, 12, 12))
#define DBG_ORANGE (QColor(247, 149, 55))

#define DBG_COLOR_BY_POS(x, y) \
	((((x / 8) % 2) == ((y / 8) % 2)) ?	\
		QColor(255, 255, 255) :			\
		QColor(204, 204, 204))

#endif /* DBGCOLORS_H */
