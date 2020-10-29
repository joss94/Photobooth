#include "PhotoboothContext.h"
#include "PhotoboothWindow.h"
#include "BackgroundSwitcher.h"

#include "QApplication"

PhotoboothContext::PhotoboothContext()
{
	_pSwitcher = new BackgroundSwitcher();

	_pWindow = new PhotoboothWindow(this);
	_pWindow->show();

	connect(_pSwitcher, &BackgroundSwitcher::backgroundSwitched, this, [&]() {
		QLabel* l = new QLabel();
		auto picture = _pSwitcher->_processedImage;
		l->setPixmap(QPixmap::fromImage(QImage(picture.data, picture.cols, picture.rows, QImage::Format_BGR888)));
		l->show();
	}, Qt::QueuedConnection);
}

PhotoboothContext::~PhotoboothContext()
{
	delete _pSwitcher;
	delete _pWindow;
}

void PhotoboothContext::onNewPicTaken(cv::Mat picture)
{
	moveToThread();

	QLabel* l = new QLabel();
	l->setPixmap(QPixmap::fromImage(QImage(picture.data, picture.cols, picture.rows, QImage::Format_BGR888)));
	l->show();

	cv::Mat background;
	cv::resize(cv::imread("C:/Dev/photobooth/Resources/backgrounds/vache.jpg"), background, cv::Size(picture.cols, picture.rows));

	new std::thread(&BackgroundSwitcher::switchBackground, _pSwitcher, picture, background);
}
