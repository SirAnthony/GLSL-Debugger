
#include "shmappingwidget.h"
#include "ui_shmappingwidget.h"
#include "mappings.h"
#include "data/dataBox.h"
#include <float.h>
#include <algorithm>


#define COLORS_COUNT 4
static QString colors[COLORS_COUNT] = {
	"black", "red", "green", "blue"
};
static QString mapping_names[COLORS_COUNT] = {
	"", "MappingRed", "MappingGreen", "MappingBlue"
};
static QString range_icons[RANGE_MAP_COUNT] = {
	"solid", "positive", "negative", "absolute"
};

static QString get_color(QString name)
{
	for (int i = 0; i < COLORS_COUNT; ++i) {
		if (!name.compare(mapping_names[i]))
			return colors[i];
	}
	return colors[0];
}


ShMappingWidget::ShMappingWidget(QWidget *parent) :
	QWidget(parent), ui(new Ui::ShMappingWidget)
{
	RangeMapping rm;
	ui->setupUi(this);
	initValButtons();

	QString color = get_color(this->objectName());
	for (int i = 0; i < RANGE_MAP_COUNT; ++i) {
		rm.index = i;
		rm.range = static_cast<RangeMap>(RANGE_MAP_DEFAULT + i);
		QString icon_name = QString(":/icons/icons/watch-" + color +
									"-" + range_icons[i] + "_32.png");
		ui->cbMap->addItem(QIcon(icon_name), QString(""), getIntFromRangeMapping(rm));
	}
}

void ShMappingWidget::addOption(int idx, QString &name)
{
	Mapping m;
	m.type = MAP_TYPE_VAR;
	m.index = idx;
	ui->cbVal->addItem(name, QVariant(getIntFromMapping(m)));
}


void ShMappingWidget::delOption(int idx)
{
	/* Check if it's in use right now */
	QVariant data = ui->cbVal->itemData(ui->cbVal->currentIndex());
	Mapping m = getMappingFromInt(data.toInt());
	if (m.index == idx)
		ui->cbVal->setCurrentIndex(0);

	/* Delete options in comboboxes */
	m.index = idx;
	m.type = MAP_TYPE_VAR;
	int map = getIntFromMapping(m);
	int didx = ui->cbVal->findData(QVariant(map));
	ui->cbVal->removeItem(didx);
	shiftOptions(didx);
	cbValActivated(0);
	emit updateScatter();
}

int ShMappingWidget::currentValIndex()
{
	return ui->cbVal->currentIndex();
}

int ShMappingWidget::currentValData()
{
	return ui->cbVal->itemData(ui->cbVal->currentIndex()).toInt();
}

int ShMappingWidget::currentMapData()
{
	return ui->cbMap->itemData(ui->cbMap->currentIndex()).toInt();
}

void ShMappingWidget::minMax(float minmax[2])
{
	float max = ui->dsMax->value();
	float min = ui->dsMin->value();
	minmax[0] = min;
	minmax[1] = fabs(max - min) > FLT_EPSILON ? max : 1.0f + min;
}

void ShMappingWidget::cbValActivated(int idx)
{
	Mapping m = getMappingFromInt(ui->cbVal->itemData(idx).toInt());
	bool enabled = m.type == MAP_TYPE_VAR;
	updateControls(enabled);

	if (!enabled) {
		ui->tbMin->setText(QString::number(0.0));
		ui->tbMax->setText(QString::number(0.0));
		ui->dsMin->setValue(0.0);
		ui->dsMax->setValue(0.0);
		ui->cbMap->setCurrentIndex(idx);
	} else {
		updateRangeMapping(idx, ui->cbMap->currentIndex());
		updateBoundaries();
		Mapping m = getMappingFromInt(ui->cbVal->itemData(idx).toInt());
		DataBox *box = NULL;
		emit getDataBox(m.index, &box);
		connect(box, SIGNAL(dataChanged()), this, SLOT(mappingDataChanged()));
	}
	updateData();
}

void ShMappingWidget::cbMapActivated(int idx)
{
	updateRangeMapping(ui->cbVal->currentIndex(), idx);
	updateData();
}

void ShMappingWidget::updateData()
{
	int idx = ui->cbVal->currentIndex();
	if (!idx) {
		emit clearData();
	} else {
		Mapping m = getMappingFromInt(ui->cbVal->itemData(idx).toInt());
		RangeMapping rm = getRangeMappingFromInt(ui->cbMap->itemData(ui->cbMap->currentIndex()).toInt());
		emit updateData(m.index, rm.range, ui->dsMin->value(), ui->dsMax->value());
	}
	emit updateScatter();
	emit updateView(true);
}

void ShMappingWidget::mappingDataChanged()
{
	DataBox *db = static_cast<DataBox*>(sender());
	if (db) {
		Mapping m = getMappingFromInt(ui->cbVal->itemData(ui->cbVal->currentIndex()).toInt());
		DataBox *active = NULL;
		emit getDataBox(m.index, &active);
		if (active != db) {
			disconnect(db, SIGNAL(dataChanged()), this, SLOT(mappingDataChanged()));
			return;
		}
	}
	updateData();
}

void ShMappingWidget::buttonClicked()
{
	double min, max, value;
	Mapping m = getMappingFromInt(ui->cbVal->itemData(ui->cbVal->currentIndex()).toInt());
	RangeMapping rm = getRangeMappingFromInt(ui->cbMap->itemData(ui->cbMap->currentIndex()).toInt());
	emit getBoundaries(m.index, &min, &max, rm.range == RANGE_MAP_ABSOLUTE);

	QDoubleSpinBox *target = NULL;
	QObject* sndr = sender();
	if (sndr == ui->tbMin) {
		target = ui->dsMin;
		value = min;
	} else if (sndr == ui->tbMax) {
		target = ui->dsMax;
		value = max;
	}

	if (target && m.type == MAP_TYPE_VAR) {
		switch (rm.range) {
			case RANGE_MAP_DEFAULT:
				target->setValue(value);
				break;
			case RANGE_MAP_POSITIVE:
				target->setValue(std::max(value, 0.0));
				break;
			case RANGE_MAP_NEGATIVE:
				target->setValue(std::min(value, 0.0));
				break;
			case RANGE_MAP_ABSOLUTE:
				target->setValue(value);
				break;
			default:
				break;
		}
	}
	updateData();
}

void ShMappingWidget::switchBoundaries()
{
	double mval = ui->dsMin->value();
	ui->dsMin->setValue(ui->dsMax->value());
	ui->dsMax->setValue(mval);
	updateData();
}

void ShMappingWidget::updateBoundaries()
{
	ui->tbMin->click();
	ui->tbMax->click();
}

void ShMappingWidget::updateRangeMapping(int idx, int ridx)
{
	Mapping m = getMappingFromInt(ui->cbVal->itemData(idx).toInt());
	RangeMapping rm = getRangeMappingFromInt(ui->cbMap->itemData(ridx).toInt());
	double min, max;
	emit getBoundaries(m.index, &min, &max, rm.range == RANGE_MAP_ABSOLUTE);
	switch (rm.range) {
	case RANGE_MAP_DEFAULT:
	case RANGE_MAP_ABSOLUTE:
		ui->tbMin->setText(QString::number(min));
		ui->tbMax->setText(QString::number(max));
		break;
	case RANGE_MAP_POSITIVE:
		ui->tbMin->setText(QString::number(std::max(min, 0.0)));
		ui->tbMax->setText(QString::number(std::max(max, 0.0)));
		break;
	case RANGE_MAP_NEGATIVE:
		ui->tbMin->setText(QString::number(std::min(min, 0.0)));
		ui->tbMax->setText(QString::number(std::min(max, 0.0)));
		break;
	default:
		break;
	}
}

void ShMappingWidget::updateControls(bool enabled)
{
	ui->tbMin->setEnabled(enabled);
	ui->tbMax->setEnabled(enabled);
	ui->dsMin->setEnabled(enabled);
	ui->dsMax->setEnabled(enabled);
	ui->cbSync->setEnabled(enabled);
	ui->cbMap->setEnabled(enabled);
	ui->tbSwitch->setEnabled(enabled);
}

void ShMappingWidget::initValButtons()
{
	Mapping m;
	m.index = 0;
	m.type = MAP_TYPE_OFF;
	ui->cbVal->addItem(QString("-"), QVariant(getIntFromMapping(m)));
	ui->cbVal->setCurrentIndex(0);
}

void ShMappingWidget::shiftOptions(int didx)
{
	Mapping m;
	for (int i = didx; i < ui->cbVal->count(); i++) {
		m = getMappingFromInt(ui->cbVal->itemData(i).toInt());
		m.index--;
		ui->cbVal->setItemData(i, getIntFromMapping(m));
	}
}



void ShMappingWidgetFragment::addOption(int idx, QString &name)
{
	Mapping mapping;
	int itemIndex;
	mapping.type = MAP_TYPE_VAR;
	mapping.index = idx;
	QVariant itemData = QVariant(getIntFromMapping(mapping));
	ui->cbVal->addItem(name, itemData);

	/* Check if this new mapping could be used */
	itemIndex = ui->cbVal->currentIndex();
	itemData = ui->cbVal->itemData(itemIndex);
	mapping = getMappingFromInt(itemData.toInt());

	if (mapping.type == MAP_TYPE_BLACK) {
		mapping.type = MAP_TYPE_VAR;
		mapping.index = idx;
		itemData = QVariant(getIntFromMapping(mapping));
		itemIndex = ui->cbVal->findData(itemData);
		ui->cbVal->setCurrentIndex(itemIndex);
		ui->tbMin->click();
		ui->tbMax->click();
	}
}

QString ShMappingWidgetFragment::asText()
{
	int index = ui->cbMap->currentIndex();
	int data = ui->cbMap->itemData(index).toInt();
	Mapping m = getMappingFromInt(data);

	switch (m.type) {
	case MAP_TYPE_BLACK:
	case MAP_TYPE_OFF:
		return "black";
	case MAP_TYPE_WHITE:
		return get_color(this->objectName());
	case MAP_TYPE_VAR: {
		RangeMapping rm = getRangeMappingFromInt(data);
		QString text = ui->cbVal->itemText(index);
		switch (rm.range) {
		case RANGE_MAP_POSITIVE:
			return "+(" + text + ")";
		case RANGE_MAP_NEGATIVE:
			return "-(" + text + ")";
		case RANGE_MAP_ABSOLUTE:
			return "|" + text + "|";
		case RANGE_MAP_DEFAULT:
		default:
			return text;
		}
		break;
	}
	}

	return "?";
}

void ShMappingWidgetFragment::initValButtons()
{
	/* Initialize color mapping comboboxes */
	Mapping m;
	m.index = 0;
	m.type = MAP_TYPE_BLACK;
	ui->cbVal->addItem(QString("black"), QVariant(getIntFromMapping(m)));

	m.index = 1;
	m.type = MAP_TYPE_WHITE;
	QString color = get_color(this->objectName());
	ui->cbVal->addItem(QString(color), QVariant(getIntFromMapping(m)));
	ui->cbVal->setCurrentIndex(0);
}
