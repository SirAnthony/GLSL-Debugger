
#ifndef FRAGMENTTEST_H
#define FRAGMENTTEST_H

#include <QDialog>

struct FragmentTestOptions {
	int alphaTest;
	int depthTest;
	int stencilTest;
	int blending;

	bool copyRGB;
	bool copyAlpha;
	bool copyDepth;
	bool copyStencil;

	float redValue;
	float greenValue;
	float blueValue;

	float alphaValue;
	float depthValue;
	int stencilValue;

	FragmentTestOptions() { reset(); }
	void reset();
};


namespace Ui {
	class ShFragmentTest;
}

class FragmentTestDialog: public QDialog {
Q_OBJECT

public:
	FragmentTestDialog(FragmentTestOptions &opts, QWidget *parent = 0);

public slots:
	void setDefaults();

private slots:
	void resetSettings();
	void apply();
	void updateStencil(int);
	void updateStencil(QString);

private:
	Ui::ShFragmentTest *ui;
	FragmentTestOptions &options;
};

#endif /* FRAGMENTTEST_H */
