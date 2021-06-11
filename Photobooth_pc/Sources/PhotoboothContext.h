#pragma once

#include "opencv2/opencv.hpp"
#include "QObject"
#include "QTimer"

class PhotoboothWindow;
class TriggerWindow;
class BackgroundSwitcher;
class MosaicBuilder;
class PhotoboothSettings;

class PhotoboothContext : public QObject
{
	Q_OBJECT

public:

	PhotoboothContext();

	~PhotoboothContext();

	PhotoboothSettings* getSettings();

	void onNewPicTaken(cv::Mat picture);

	QString getTimestamp();

	void onSettingsUpdate();

	QString getRootFolder();

	void takePicture();

	void refreshMosaic();

public slots:

	void onBackgroundSwitched(cv::Mat image);

	void onRouletteFinished(cv::Mat image);

	void onMosaicUpdated(cv::Mat image, bool finished);

private:

	PhotoboothSettings* _pSettings = nullptr;

	PhotoboothWindow* _pWindow = nullptr;
	TriggerWindow* _pTriggerWindow = nullptr;
	BackgroundSwitcher* _pSwitcher = nullptr;
	MosaicBuilder* _pBuilder = nullptr;

	QTimer _mosaicRefreshTimer;
};