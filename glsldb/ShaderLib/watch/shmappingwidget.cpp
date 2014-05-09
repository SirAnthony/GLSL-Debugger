
#include "shmappingwidget.h"
#include "ui_shmappingwidget.h"
#include "mappings.h"
#include "data/vertexBox.h"
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


ShMappingWidget::ShMappingWidget(QWidget *parent) :
	QWidget(parent)
{	
	Mapping m;
	RangeMapping rm;
	ui->setupUi(this);

	m.index = 0;
	m.type = MAP_TYPE_OFF;
	ui->cbVal->addItem(QString("-"), QVariant(getIntFromMapping(m)));
	ui->cbVal->setCurrentIndex(0);

	QString name = this->objectName();
	QString color = colors[0];
	for (int i = 0; i < COLORS_COUNT; ++i) {
		if (!name.compare(mapping_names[i])) {
			color = colors[i];
			break;
		}
	}

	for (int i = 0; i < RANGE_MAP_COUNT; ++i) {
		rm.index = i;
		rm.range = static_cast<RangeMap>(RANGE_MAP_DEFAULT + i);
		QString icon_name = QString(":/icons/icons/watch-" + color +
									"-" + range_icons[i] + "_32.png");
		ui->cbMap->addItem(QIcon(icon_name), QString(""), getIntFromRangeMapping(rm));
	}
}

void ShMappingWidget::addOption(int idx, QString name)
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
	if (m.index == idx) {
		ui->cbVal->setCurrentIndex(0);
		cbValActivated(0);
	}

	/* Delete options in comboboxes */
	m.index = idx;
	m.type = MAP_TYPE_VAR;
	int map = getIntFromMapping(m);
	int didx = ui->cbVal->findData(QVariant(map));
	ui->cbVal->removeItem(didx);
	for (int i = didx; i < ui->cbVal->count(); i++) {
		m = getMappingFromInt(ui->cbVal->itemData(i).toInt());
		m.index--;
		ui->cbVal->setItemData(i, getIntFromMapping(m));
	}
	emit updateScatter();
}


void ShMappingWidget::cbValActivated(int idx)
{
	bool enabled = bool(idx);
	ui->tbMin->setEnabled(enabled);
	ui->tbMax->setEnabled(enabled);
	ui->dsMin->setEnabled(enabled);
	ui->dsMax->setEnabled(enabled);
	ui->cbSync->setEnabled(enabled);
	ui->cbMap->setEnabled(enabled);
	ui->tbSwitch->setEnabled(enabled);

	if (!idx) {
		ui->tbMin->setText(QString::number(0.0));
		ui->tbMax->setText(QString::number(0.0));
		ui->dsMin->setValue(0.0);
		ui->dsMax->setValue(0.0);
		ui->cbMap->setCurrentIndex(0);
	} else {
		updateRangeMapping(idx, ui->cbMap->currentIndex());
		ui->tbMin->click();
		ui->tbMax->click();
		Mapping m = getMappingFromInt(ui->cbVal->itemData(idx).toInt());
		VertexBox *vb = model->getDataColumn(m.index);
		connect(vb, SIGNAL(dataChanged()), this, SLOT(mappingDataChanged()));
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
	VertexBox *vb = static_cast<VertexBox*>(sender());
	if (vb) {
		Mapping m = getMappingFromInt(ui->cbVal->itemData(ui->cbVal->currentIndex()).toInt());
		VertexBox *active = model->getDataColumn(m.index);
		if (active != vb) {
			disconnect(vb, SIGNAL(dataChanged()), this, SLOT(mappingDataChanged()));
			return;
		}
	}
	updateData();
}

void ShMappingWidget::buttonClicked()
{
	Mapping m = getMappingFromInt(ui->cbVal->itemData(ui->cbVal->currentIndex()).toInt());
	RangeMapping rm = getRangeMappingFromInt(ui->cbMap->itemData(ui->cbMap->currentIndex()).toInt());

	QDoubleSpinBox *target = NULL;
	QObject* sndr = sender();
	if (sndr == ui->tbMin)
		target = ui->dsMin;
	else if (sndr == ui->tbMax)
		target = ui->dsMax;

	VertexBox *vb = model->getDataColumn(m.index);
	if (target && m.type == MAP_TYPE_VAR) {
		switch (rm.range) {
			case RANGE_MAP_DEFAULT:
				target->setValue(vb->getMin());
				break;
			case RANGE_MAP_POSITIVE:
				target->setValue(std::max(vb->getMin(), 0.0));
				break;
			case RANGE_MAP_NEGATIVE:
				target->setValue(std::min(vb->getMin(), 0.0));
				break;
			case RANGE_MAP_ABSOLUTE:
				target->setValue(vb->getAbsMin());
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

void ShMappingWidget::updateRangeMapping(int idx, int ridx)
{
	Mapping m = getMappingFromInt(ui->cbVal->itemData(idx).toInt());
	RangeMapping rm = getRangeMappingFromInt(ui->cbMap->itemData(ridx).toInt());
	VertexBox *vb = model->getDataColumn(m.index);
	switch (rm.range) {
	case RANGE_MAP_DEFAULT:
		ui->tbMin->setText(QString::number(vb->getMin()));
		ui->tbMax->setText(QString::number(vb->getMax()));
		break;
	case RANGE_MAP_POSITIVE:
		ui->tbMin->setText(QString::number(std::max(vb->getMin(), 0.0)));
		ui->tbMax->setText(QString::number(std::max(vb->getMax(), 0.0)));
		break;
	case RANGE_MAP_NEGATIVE:
		ui->tbMin->setText(QString::number(std::min(vb->getMin(), 0.0)));
		ui->tbMax->setText(QString::number(std::min(vb->getMax(), 0.0)));
		break;
	case RANGE_MAP_ABSOLUTE:
		ui->tbMin->setText(QString::number(vb->getAbsMin()));
		ui->tbMax->setText(QString::number(vb->getAbsMax()));
		break;
	default:
		break;
	}
}
