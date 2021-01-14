#include "PhotoboothContext.h"
#include "PhotoboothWindow.h"
#include "BackgroundSwitcher.h"

#include "QApplication"

PhotoboothContext::PhotoboothContext()
{
	_pSwitcher = new BackgroundSwitcher();

	_pWindow = new PhotoboothWindow(this);
	_pWindow->show();

	connect(_pWindow, &PhotoboothWindow::picTakenSignal,
		this, &PhotoboothContext::onNewPicTaken);
	
	connect(_pSwitcher, &BackgroundSwitcher::backgroundSwitched, 
		this, &PhotoboothContext::onBackgroundSwitched);

	connect(_pSwitcher, &BackgroundSwitcher::imageProcessed,
		this, [&]() {_pSwitcher->switchBackgroundRoulette("C:/backgrounds"); });

}

PhotoboothContext::~PhotoboothContext()
{
	delete _pSwitcher;
	delete _pWindow;
}

void PhotoboothContext::onNewPicTaken(cv::Mat picture)
{
	_pWindow->showImage(QImage(picture.data, picture.cols, picture.rows, QImage::Format_BGR888));

	cv::Mat background;
	cv::resize(cv::imread("C:/Dev/photobooth/Resources/backgrounds/vache.jpg"), background, cv::Size(picture.cols, picture.rows));

	//new std::thread(&BackgroundSwitcher::switchBackground, _pSwitcher, picture, background);
	new std::thread([&](cv::Mat pic) 
	{
		_pSwitcher->processNewFrame(pic);
	}, picture);
}

void PhotoboothContext::onBackgroundSwitched(cv::Mat image)
{
	_pWindow->showImage(QImage(image.data, image.cols, image.rows, QImage::Format_BGR888));
}
