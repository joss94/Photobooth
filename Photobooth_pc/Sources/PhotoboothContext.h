#pragma once

#include "opencv2/opencv.hpp"
#include "QObject"

class PhotoboothWindow;
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

public slots:

	void onBackgroundSwitched(cv::Mat image);

	void onRouletteFinished(cv::Mat image);

	void onShowMosaicClicked();

	void onMosaicUpdated(cv::Mat image);

private:

	PhotoboothSettings* _pSettings = nullptr;

	PhotoboothWindow* _pWindow = nullptr;
	BackgroundSwitcher* _pSwitcher = nullptr;
	MosaicBuilder* _pBuilder = nullptr;
};