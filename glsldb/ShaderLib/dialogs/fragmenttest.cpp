
#include "fragmenttest.h"
#include "ui_fragmenttestdialog.h"
#include "debuglib.h"

static int mapPFTToIndex(int pftOption)
{
	switch (pftOption) {
	case DBG_PFT_FORCE_DISABLED:
		return 1;
	case DBG_PFT_FORCE_ENABLED:
		return 2;
	case DBG_PFT_KEEP:
	default:
		return 0;
	}
}

static int mapIndexToPFT(int index)
{
	switch (index) {
	case 1:
		return DBG_PFT_FORCE_DISABLED;
	case 2:
		return DBG_PFT_FORCE_ENABLED;
	case 0:
	default:
		return DBG_PFT_KEEP;
	}
}

void FragmentTestOptions::reset()
{
	alphaTest = DBG_PFT_KEEP;
	depthTest = DBG_PFT_KEEP;
	stencilTest = DBG_PFT_KEEP;
	blending = DBG_PFT_FORCE_DISABLED;

	copyRGB = false;
	copyAlpha = true;
	copyDepth = true;
	copyStencil = true;

	redValue = 0.0;
	greenValue = 0.0;
	blueValue = 0.0;

	alphaValue = 0.0;
	depthValue = 0.0;
	stencilValue = 0;
}

static const QRegExp hex_regexp("[0-9a-fA-F]{0,4}");
static const QRegExpValidator hex_validator(hex_regexp);

FragmentTestDialog::FragmentTestDialog(FragmentTestOptions &opts, QWidget *parent) :
	 QDialog(parent), ui(new Ui::ShFragmentTest), options(opts)
{
	ui->setupUi(this);
	// TODO: masks
	ui->gbMasks->setVisible(false);
	ui->leStencilValue->setValidator(&hex_validator);
	resetSettings();
}

void FragmentTestDialog::setDefaults()
{
	options.reset();
	resetSettings();
}

void FragmentTestDialog::resetSettings()
{
	ui->cbAlphaTest->setCurrentIndex(mapPFTToIndex(options.alphaTest));
	ui->cbDepthTest->setCurrentIndex(mapPFTToIndex(options.depthTest));
	ui->cbStencilTest->setCurrentIndex(mapPFTToIndex(options.stencilTest));
	ui->cbBlending->setCurrentIndex(mapPFTToIndex(options.blending));

	ui->cbRGBCopy->setChecked(options.copyRGB);
	ui->cbAlphaCopy->setChecked(options.copyAlpha);
	ui->cbDepthCopy->setChecked(options.copyDepth);
	ui->cbStencilCopy->setChecked(options.copyStencil);

	ui->dsRedValue->setValue(options.redValue);
	ui->dsGreenValue->setValue(options.greenValue);
	ui->dsBlueValue->setValue(options.blueValue);

	ui->dsAlphaValue->setValue(options.alphaValue);
	ui->dsDepthValue->setValue(options.depthValue);
	ui->dsStencilValue->setValue(options.stencilValue);
	ui->leStencilValue->setText(QString::number(options.stencilValue, 16));
}

void FragmentTestDialog::apply()
{
	options.alphaTest = mapIndexToPFT(ui->cbAlphaTest->currentIndex());
	options.depthTest = mapIndexToPFT(ui->cbDepthTest->currentIndex());
	options.stencilTest = mapIndexToPFT(ui->cbStencilTest->currentIndex());
	options.blending = mapIndexToPFT(ui->cbBlending->currentIndex());

	options.copyRGB = ui->cbRGBCopy->isChecked();
	options.copyAlpha = ui->cbAlphaCopy->isChecked();
	options.copyDepth = ui->cbDepthCopy->isChecked();
	options.copyStencil = ui->cbStencilCopy->isChecked();

	options.redValue = ui->dsRedValue->value();
	options.greenValue = ui->dsGreenValue->value();
	options.blueValue = ui->dsBlueValue->value();

	options.alphaValue = ui->dsAlphaValue->value();
	options.depthValue = ui->dsDepthValue->value();
	options.stencilValue = ui->dsStencilValue->value();

	accept();
}

void FragmentTestDialog::updateStencil(int value)
{
	ui->leStencilValue->setText(QString::number(value, 16));
}

void FragmentTestDialog::updateStencil(QString value)
{
	ui->dsStencilValue->setValue(value.toInt(NULL, 16));
}
