
#include "shmappingwidget.h"
#include "ui_shmappingwidget.h"
#include "mappings.h"
#include "data/vertexBox.h"


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
		updateRangeMapping(ui->cbMap->currentIndex());
		ui->tbMin->click();
		ui->tbMax->click();
		Mapping m = getMappingFromInt(ui->cbVal->itemData(idx).toInt());
		connect(model->getDataColumn(m.index), SIGNAL(dataChanged()),
				this, SLOT(mappingDataChanged()));
	}
	updateData();
}

void ShMappingWidget::cbMapActivated(int idx)
{
	updateRangeMapping(idx);
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
	VertexBox *sendingVB = static_cast<VertexBox*>(sender());
	if (sendingVB) {
		Mapping m = getMappingFromInt(ui->cbVal->itemData(ui->cbVal->currentIndex()).toInt());
		VertexBox *activeVB = NULL;
		activeVB = model->getDataColumn(m.index);
		if (activeVB != sendingVB) {
			disconnect(sendingVB, SIGNAL(dataChanged()), this, SLOT(mappingDataChanged()));
			return;
		}
	}
	/* It probably works as updateData.
		int rmindex = ui->cbMap->currentIndex();
		updateRangeMapping(rmindex);
		RangeMapping rm = getRangeMappingFromInt(rmindex);
		updateDataCurrent(m_scatterData##VAL, &m_scatterDataCount##VAL, m_scatterDataStride##VAL,
						sendingVB, &m, &rm, ui->dsMin->value(), ui->dsMax->value());
		emit updateScatter();
	*/
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

	if (target && m.type == MAP_TYPE_VAR) {
		switch (rm.range) {
			case RANGE_MAP_DEFAULT:
				target->setValue(m_vertexTable->getDataColumn(m.index)->getMin());
				break;
			case RANGE_MAP_POSITIVE:
				target->setValue(MAX(m_vertexTable->getDataColumn(m.index)->getMin(), 0.0));
				break;
			case RANGE_MAP_NEGATIVE:
				target->setValue(MIN(m_vertexTable->getDataColumn(m.index)->getMin(), 0.0));
				break;
			case RANGE_MAP_ABSOLUTE:
				target->setValue(m_vertexTable->getDataColumn(m.index)->getAbsMin());
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

void ShMappingWidget::updateRangeMapping(int idx)
{
	RangeMapping rm = getRangeMappingFromInt(ui->cbMap->itemData(idx).toInt());
	switch (rm.range) {
	case RANGE_MAP_DEFAULT:
		ui->tbMin->setText(QString::number(m_vertexTable->getDataColumn(m.index)->getMin()));
		ui->tbMax->setText(QString::number(m_vertexTable->getDataColumn(m.index)->getMax()));
		break;
	case RANGE_MAP_POSITIVE:
		ui->tbMin->setText(QString::number(MAX(m_vertexTable->getDataColumn(m.index)->getMin(), 0.0)));
		ui->tbMax->setText(QString::number(MAX(m_vertexTable->getDataColumn(m.index)->getMax(), 0.0)));
		break;
	case RANGE_MAP_NEGATIVE:
		ui->tbMin->setText(QString::number(MIN(m_vertexTable->getDataColumn(m.index)->getMin(), 0.0)));
		ui->tbMax->setText(QString::number(MIN(m_vertexTable->getDataColumn(m.index)->getMax(), 0.0)));
		break;
	case RANGE_MAP_ABSOLUTE:
		ui->tbMin->setText(QString::number(m_vertexTable->getDataColumn(m.index)->getAbsMin()));
		ui->tbMax->setText(QString::number(m_vertexTable->getDataColumn(m.index)->getAbsMax()));
		break;
	default:
		break;
	}
}
