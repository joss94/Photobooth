#include "PhotoboothContext.h"
#include "PhotoboothWindow.h"
#include "TriggerWindow.h"
#include "BackgroundSwitcher.h"

#include "QApplication"
#include "MosaicBuilder.h"
#include "PhotoboothSettings.h"
#include <QDir>
#include <QFileDialog>


QString correctPath(QString path)
{
	return path;
	QStringList pieces = path.split("/");
	QString output = "";
	for (auto& p : pieces)
	{
		if (p.contains(" "))
		{
			output += QString("\"") + p + QString("\"");
		}
		else
		{
			output += p;
		}

		output += "/";
	}

	return output.left(output.size() - 1);
}

PhotoboothContext::PhotoboothContext()
{
	_pSettings = new PhotoboothSettings(QCoreApplication::applicationDirPath() + "/settings.json");

	qDebug() << "Pictures directory: " << getRootFolder() + "/pictures";
	qDebug() << "Modified pictures directory: " << getRootFolder() + "/pictures/modified";
	qDebug() << "Backgrounds directory: " << getRootFolder() + "/backgrounds";

	QDir().mkpath(correctPath(getRootFolder() + "/pictures"));
	QDir().mkpath(correctPath(getRootFolder() + "/pictures/modified"));
	QDir().mkpath(correctPath(getRootFolder() + "/backgrounds"));

	_pSwitcher = new BackgroundSwitcher(_pSettings->settings().scriptPath, _pSettings->settings().modelPath);
	_pBuilder = new MosaicBuilder();

	// Make sure we get an existing base image
	while (!QFile::exists(_pSettings->settings().baseImagePath))
	{
		auto sett = _pSettings->settings();
		sett.baseImagePath = QFileDialog::getOpenFileName(nullptr, tr("Select base image"), "", tr("ImageFiles (*.png *.jpg *.bmp)"));
		_pSettings->updateSettings(sett);
	}

	onSettingsUpdate();

	_pWindow = new PhotoboothWindow(this);
	_pWindow->show();

	_pTriggerWindow = new TriggerWindow(this);
	_pTriggerWindow->show();

	_mosaicRefreshTimer.setSingleShot(true);
	connect(&_mosaicRefreshTimer, &QTimer::timeout, this, [&]() {
		_pBuilder->refreshImage();
		});

	connect(_pWindow, &PhotoboothWindow::picTakenSignal,this, [&]() {
			_mosaicRefreshTimer.stop();
		});

	connect(_pWindow, &PhotoboothWindow::picTakenSignal,
		this, &PhotoboothContext::onNewPicTaken);
	
	connect(_pSwitcher, &BackgroundSwitcher::backgroundSwitched, 
		this, &PhotoboothContext::onBackgroundSwitched);

	connect(_pSwitcher, &BackgroundSwitcher::rouletteFinished,
		this, &PhotoboothContext::onRouletteFinished);

	connect(_pSwitcher, &BackgroundSwitcher::imageProcessed,
		this, [&]() {_pSwitcher->switchBackgroundRoulette(getRootFolder() + "/backgrounds"); });

	connect(_pBuilder, &MosaicBuilder::mosaicUpdatedSignal,
		this, &PhotoboothContext::onMosaicUpdated);
}

PhotoboothContext::~PhotoboothContext()
{
	delete _pBuilder;
	delete _pSwitcher;
	delete _pWindow;
	delete _pTriggerWindow;
	delete _pSettings;
}

PhotoboothSettings* PhotoboothContext::getSettings()
{
	return _pSettings;
}

QString PhotoboothContext::getTimestamp()
{
	return QDateTime::currentDateTime().toString("yyyy_MM_dd_hh_mm_ss");
}

void PhotoboothContext::onSettingsUpdate()
{
	_pBuilder->setMaxOccurence(_pSettings->settings().maxOccurence);
	_pBuilder->setBaseOpacity(_pSettings->settings().baseOpacity);
	_pBuilder->setDisplaySpeed(_pSettings->settings().displaySpeed);
	_pBuilder->setMosaicSize(_pSettings->settings().mosaicWidth, _pSettings->settings().mosaicHeight);
	_pBuilder->setBaseImage(_pSettings->settings().baseImagePath);
	_pBuilder->setTilesDirectory(getRootFolder() + "/pictures/modified");
}

QString PhotoboothContext::getRootFolder()
{
	return QCoreApplication::applicationDirPath();
}

void PhotoboothContext::takePicture()
{
	_pWindow->clickShutterButton();
}

void PhotoboothContext::refreshMosaic()
{
	_pBuilder->refreshImage();
}

void PhotoboothContext::onNewPicTaken(cv::Mat picture)
{
	QString path = getRootFolder() + "/pictures/pic_" + getTimestamp() + ".jpg";

	// imwrite crashes in debug mode, I don't know why
#ifdef NDEBUG
	cv::imwrite(path.toStdString(), picture);
#endif

	_pWindow->showImage(QImage(picture.data, picture.cols, picture.rows, QImage::Format_BGR888));
	_pWindow->showWaitMessage("Embellissement<br/>en cours...");
	_pSwitcher->processNewFrame(picture);
}

void PhotoboothContext::onBackgroundSwitched(cv::Mat image)
{
	_pWindow->hideWaitMessage();
	_pWindow->showImage(QImage(image.data, image.cols, image.rows, QImage::Format_BGR888));
}

void PhotoboothContext::onRouletteFinished(cv::Mat image)
{
	_pWindow->hideWaitMessage();
	QString path = getRootFolder() + "/pictures/modified" + "/pic_" + getTimestamp() + ".jpg";
	
	// imwrite crashes in debug mode, I don't know why
	#ifdef NDEBUG
	cv::imwrite(path.toStdString(), image);
	#endif

	_mosaicRefreshTimer.start(_pSettings->settings().delayBeforeMosaic);
	_pWindow->setReadyForPicture(true);
}

void PhotoboothContext::onMosaicUpdated(cv::Mat image, bool finished)
{
	_pWindow->showImage(QImage(image.data, image.cols, image.rows, QImage::Format_BGR888));
}
