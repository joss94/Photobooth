#include "PhotoboothContext.h"
#include "PhotoboothWindow.h"
#include "BackgroundSwitcher.h"

#include "QApplication"
#include "MosaicBuilder.h"
#include "PhotoboothSettings.h"
#include "QDir"

PhotoboothContext::PhotoboothContext()
{
	_pSettings = new PhotoboothSettings(QCoreApplication::applicationDirPath() + "/settings.json");

	qDebug() << "Pictures directory: " << _pSettings->settings().picturesDir;
	qDebug() << "Modified pictures directory: " << _pSettings->settings().modifPicturesDir;
	qDebug() << "Backgrounds directory: " << _pSettings->settings().backgroundDir;

	QDir().mkpath(_pSettings->settings().picturesDir);
	QDir().mkpath(_pSettings->settings().modifPicturesDir);

	_pSwitcher = new BackgroundSwitcher(_pSettings->settings().scriptPath, _pSettings->settings().modelPath);

	_pBuilder = new MosaicBuilder();
	_pBuilder->setBaseImage(_pSettings->settings().baseImagePath);
	_pBuilder->setTilesDirectory(_pSettings->settings().modifPicturesDir);
	_pBuilder->setMaxOccurence(_pSettings->settings().maxOccurence);
	_pBuilder->setBaseOpacity(_pSettings->settings().baseOpacity);

	_pWindow = new PhotoboothWindow(this);
	_pWindow->showFullScreen();

	connect(_pWindow, &PhotoboothWindow::picTakenSignal,
		this, &PhotoboothContext::onNewPicTaken);
	
	connect(_pSwitcher, &BackgroundSwitcher::backgroundSwitched, 
		this, &PhotoboothContext::onBackgroundSwitched);

	connect(_pSwitcher, &BackgroundSwitcher::rouletteFinished,
		this, &PhotoboothContext::onRouletteFinished);

	connect(_pSwitcher, &BackgroundSwitcher::imageProcessed,
		this, [&]() {_pSwitcher->switchBackgroundRoulette(_pSettings->settings().backgroundDir); });

	connect(_pBuilder, &MosaicBuilder::mosaicUpdatedSignal,
		this, &PhotoboothContext::onMosaicUpdated);
}

PhotoboothContext::~PhotoboothContext()
{
	delete _pBuilder;
	delete _pSwitcher;
	delete _pWindow;
	delete _pSettings;
}

PhotoboothSettings* PhotoboothContext::getSettings()
{
	return _pSettings;
}

void PhotoboothContext::onNewPicTaken(cv::Mat picture)
{
	QString path = _pSettings->settings().picturesDir + "/pic_" + getTimestamp() + ".jpg";

	// imwrite crashes in debug mode, I don't know why
	#ifdef NDEBUG
	cv::imwrite(path.toStdString(), picture);
	#endif
	
	_pWindow->showImage(QImage(picture.data, picture.cols, picture.rows, QImage::Format_BGR888));
	_pSwitcher->processNewFrame(picture);
	//new std::thread([&](cv::Mat pic) 
	//{
	//	_pSwitcher->processNewFrame(pic);
	//}, picture);
}

QString PhotoboothContext::getTimestamp()
{
	return QDateTime::currentDateTime().toString("yyyy_MM_dd_hh_mm_ss");
}

void PhotoboothContext::onBackgroundSwitched(cv::Mat image)
{
	_pWindow->showImage(QImage(image.data, image.cols, image.rows, QImage::Format_BGR888));
}

void PhotoboothContext::onRouletteFinished(cv::Mat image)
{
	QString path = _pSettings->settings().modifPicturesDir + "/pic_" + getTimestamp() + ".jpg";
	
	// imwrite crashes in debug mode, I don't know why
	#ifdef NDEBUG
	cv::imwrite(path.toStdString(), image);
	#endif

	onShowMosaicClicked();
}

void PhotoboothContext::onShowMosaicClicked()
{
	cv::Mat mosaic = _pBuilder->refreshImage();
	//_pWindow->showImage(QImage(mosaic.data, mosaic.cols, mosaic.rows, QImage::Format_BGR888));
}

void PhotoboothContext::onMosaicUpdated(cv::Mat image)
{
	_pWindow->showImage(QImage(image.data, image.cols, image.rows, QImage::Format_BGR888));
}
