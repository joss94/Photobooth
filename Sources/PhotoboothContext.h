#pragma once

#include "opencv2/opencv.hpp"
#include "QObject"

class PhotoboothWindow;
class BackgroundSwitcher;
class MosaicBuilder;

class PhotoboothContext : public QObject
{
	Q_OBJECT

public:

	PhotoboothContext();

	~PhotoboothContext();

	void onNewPicTaken(cv::Mat picture);

public slots:

	void onBackgroundSwitched(cv::Mat image);

	void onShowMosaicClicked();

private:

	PhotoboothWindow* _pWindow = nullptr;
	BackgroundSwitcher* _pSwitcher = nullptr;
	MosaicBuilder* _pBuilder = nullptr;
};