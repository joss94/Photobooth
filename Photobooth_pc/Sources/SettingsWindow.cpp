#include "windows.h"

#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

#include "SettingsWindow.h"
#include "PhotoboothContext.h"

#include <QDebug>
#include <QString>
#include <QFileDialog>
#include <QEventLoop>
#include "qevent.h"
#include "PhotoboothSettings.h"

SettingsWindow::SettingsWindow(PhotoboothContext* ctx, QWidget* parent) : QWidget(parent)
{
	_pCtx = ctx;
	init();
	resetParams();
}

void SettingsWindow::init()
{
	_formLayout.addRow("DNN path", &_dnnPathEdit);
	_formLayout.addRow("DNN script", &_dnnScriptPathEdit);
	_formLayout.addRow("Camera URL", &_cameraUrlEdit);
	_formLayout.addRow("Auto IP", &_autoIpCheckBox);
	_formLayout.addRow("Base image", &_baseImagePathEdit);
	_formLayout.addRow("Max. occurence", &_maxOccurenceEdit);
	_formLayout.addRow("Base opacity", &_baseOpacityEdit);
	_formLayout.addRow("Mosaic width", &_mosaicWidthEdit);
	_formLayout.addRow("Mosaic height", &_mosaicHeightEdit);
	_formLayout.addRow("Display speed", &_displaySpeedEdit);
	_formLayout.addRow("Delay before mosaic (ms)", &_delayMosaicEdit);
	_formLayout.addRow("Second button", &_secondButtonCheckBox);
	_formLayout.addRow("Show console", &_showConsoleCheckBox);

	_dnnPathEdit.setEnabled(false);
	_dnnScriptPathEdit.setEnabled(false);

	_mainLayout.addLayout(&_formLayout);

	_mainLayout.addWidget(&_saveButton);
	connect(&_saveButton, &QPushButton::clicked, this, &SettingsWindow::onSaveButtonClicked);

	setLayout(&_mainLayout);
}

void SettingsWindow::resetParams()
{
	auto s = _pCtx->getSettings()->settings();

	_dnnPathEdit.setText(s.modelPath);
	_dnnScriptPathEdit.setText(s.scriptPath);
	_cameraUrlEdit.setText(s.cameraURL);
	_autoIpCheckBox.setCheckState(s.autoIp ? Qt::Checked : Qt::Unchecked);
	_baseImagePathEdit.setText(s.baseImagePath);
	_maxOccurenceEdit.setText(QString::number(s.maxOccurence));
	_baseOpacityEdit.setText(QString::number(s.baseOpacity));
	_mosaicWidthEdit.setText(QString::number(s.mosaicWidth));
	_mosaicHeightEdit.setText(QString::number(s.mosaicHeight));
	_displaySpeedEdit.setText(QString::number(s.displaySpeed));
	_delayMosaicEdit.setText(QString::number(s.delayBeforeMosaic));
	_secondButtonCheckBox.setCheckState(s.secondButton ? Qt::Checked : Qt::Unchecked);
	_showConsoleCheckBox.setCheckState(s.showConsole ? Qt::Checked : Qt::Unchecked);

	_saveButton.setText("Apply");
}

void SettingsWindow::onSaveButtonClicked()
{
	auto s = _pCtx->getSettings()->settings();

	s.modelPath = _dnnPathEdit.text();
	s.scriptPath = _dnnScriptPathEdit.text();
	s.cameraURL = _cameraUrlEdit.text();
	s.autoIp = _autoIpCheckBox.isChecked();
	s.baseImagePath = _baseImagePathEdit.text();

	// Make sure we get an existing base image (10 attempts, otherwise close settings window)
	int count = 0;
	while (!QFile::exists(s.baseImagePath))
	{
		s.baseImagePath = QFileDialog::getOpenFileName(nullptr, tr("Select base image"), "", tr("ImageFiles (*.png *.jpg *.bmp)"));
		count++;
		if (count > 10)
		{
			close();
			return;
		}
	}

	s.maxOccurence = _maxOccurenceEdit.text().toInt();
	s.baseOpacity = _baseOpacityEdit.text().toDouble();
	s.mosaicWidth = _mosaicWidthEdit.text().toInt();
	s.mosaicHeight = _mosaicHeightEdit.text().toInt();
	s.displaySpeed = _displaySpeedEdit.text().toInt();
	s.delayBeforeMosaic = _delayMosaicEdit.text().toInt();
	s.secondButton = _secondButtonCheckBox.isChecked();
	s.showConsole = _showConsoleCheckBox.isChecked();

	_pCtx->getSettings()->updateSettings(s);
	_pCtx->onSettingsUpdate();
	close();
}