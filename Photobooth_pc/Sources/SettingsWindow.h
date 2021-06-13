#pragma once

#include "QWidget"

#include <QStackedLayout>
#include "QBoxLayout"
#include "VideoInput.h"
#include "QPushButton"

#include <QTimer>
#include <QBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>

class PhotoboothContext;

class SettingsWindow : public QWidget
{
	Q_OBJECT

public:

	SettingsWindow(PhotoboothContext* ctx, QWidget* parent = nullptr);

	void onSaveButtonClicked();

private:

	void init();
	void resetParams();

private:

	PhotoboothContext* _pCtx = nullptr;

	QVBoxLayout _mainLayout;
	QFormLayout _formLayout;

	QLineEdit _dnnPathEdit;
	QLineEdit _dnnScriptPathEdit;
	QLineEdit _cameraUrlEdit;
	QCheckBox _autoIpCheckBox;
	QLineEdit _baseImagePathEdit;
	QLineEdit _maxOccurenceEdit;
	QLineEdit _baseOpacityEdit;
	QLineEdit _mosaicWidthEdit;
	QLineEdit _mosaicHeightEdit;
	QLineEdit _displaySpeedEdit;
	QLineEdit _delayMosaicEdit;
	QCheckBox _secondButtonCheckBox;
	QCheckBox _showConsoleCheckBox;

	QPushButton _saveButton;
};