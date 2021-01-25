#include "PhotoboothContext.h"
#include "PhotoboothWindow.h"
#include "BackgroundSwitcher.h"

#include "QApplication"
#include "MosaicBuilder.h"

PhotoboothContext::PhotoboothContext()
{
	_pSwitcher = new BackgroundSwitcher();
	_pBuilder = new MosaicBuilder();

	cv::Mat base = cv::imread("C:\\Users\\josselin_manceau\\Desktop\\Perso\\Photobooth\\photos\\IMG_20190424_121718.jpg");
	_pBuilder->setBaseImage(base);
	_pBuilder->setTilesDirectory("C:\\Users\\josselin_manceau\\Desktop\\Perso\\Photobooth\\photos_finales\\");

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
	delete _pBuilder;
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

void PhotoboothContext::onShowMosaicClicked()
{
	cv::Mat mosaic = _pBuilder->refreshImage();
	_pWindow->showImage(QImage(mosaic.data, mosaic.cols, mosaic.rows, QImage::Format_BGR888));
}
