#include "windows.h"

#include "PhotoboothWindow.h"
#include "PhotoboothContext.h"

#include <QDebug>
#include <QEventLoop>
#include "qevent.h"
#include "PhotoboothSettings.h"

PhotoboothWindow::PhotoboothWindow(PhotoboothContext* ctx, QWidget* parent) : QWidget(parent)
{
	_pCtx = ctx;

	//_testPic = cv::imread("C:/users/josselin_manceau/Desktop/Perso/Photobooth/joss_et_oriane.jpg");

	_countdownTimer.setSingleShot(true);
	connect(&_countdownTimer, &QTimer::timeout, this, [&]()
	{
		_countdownValue--;
		_countdownLabel.setText(QString("<font color='white'>" + QString::number(_countdownValue) + "</font>"));
		if (_countdownValue > 0)
		{
			showCountdown();
			_countdownTimer.start(1000);
		}
		else
		{
			cv::Mat pic = takePicture();
			//cv::Mat pic = _testPic;

			if (pic.rows > 0)
			{
				emit picTakenSignal(pic.clone());
			}

			_takingPicture = false;
			hideCountdown();
		}
	});

	_instructionsLabel.setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	_instructionsLabel.setScaledContents(true);
	QPixmap p;
	p.load("C:/Dev/photobooth/Resources/instructions.jpg");
	_instructionsLabel.setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	_instructionsLabel.setPixmap(p);
	_stackedLayout.addWidget(&_instructionsLabel);

	QFont font = _countdownLabel.font();
	font.setPointSize(152);
	font.setBold(true);
	_countdownLabel.setFont(font);
	_countdownLabel.setAlignment(Qt::AlignCenter);
	_stackedLayout.addWidget(&_countdownLabel);

	_stackedLayout.addWidget(&_frozenImageLabel);
	_stackedLayout.setCurrentWidget(&_instructionsLabel);

	_mainLayout.addLayout(&_stackedLayout);

	setLayout(&_mainLayout);

	_takePicButton = new QPushButton(this);
	_takePicButton->setGeometry(50, 50, 100, 100);
	_takePicButton->setIcon(QIcon(QPixmap("C:/Dev/photobooth/Resources/camera.png")));
	_takePicButton->setIconSize(QSize(70, 70));
	_takePicButton->show();
	_takePicButton->raise();

	_takePicButton->setStyleSheet("\
		QPushButton {\
			background-color: #fff;\
			border: 2px solid #555;\
			border-radius: 50px;\
			padding: 5px;\
		}\
		\
		QPushButton:hover{\
			background-color: #888;\
		}");

	connect(_takePicButton, &QPushButton::clicked, this, [&]() 
	{ 
		//_pCtx->onShowMosaicClicked();
		if (!_takingPicture)
		{
			_takingPicture = true;
			_countdownValue = 4;
			_countdownTimer.start(1);
		}
	});
}

void PhotoboothWindow::showImage(const QImage& image)
{
	_frozenImageLabel.setScaledContents(true);
	_frozenImageLabel.setPixmap(QPixmap::fromImage(image).scaled(_frozenImageLabel.size()));
	_stackedLayout.setCurrentWidget(&_frozenImageLabel);
	_takePicButton->raise();
}

void PhotoboothWindow::showInstuctions()
{
	_stackedLayout.setCurrentWidget(&_instructionsLabel);
	_takePicButton->raise();
	_stackedLayout.setStackingMode(QStackedLayout::StackOne);
}

void PhotoboothWindow::resizeEvent(QResizeEvent* event)
{
	QWidget::resizeEvent(event);

	if (_takePicButton != nullptr)
	{
		_takePicButton->setGeometry(
			(event->size().width() - _takePicButton->width()) / 2,
			(event->size().height() - _takePicButton->height()) - 20,
			_takePicButton->width(),
			_takePicButton->height());
	}
}

void PhotoboothWindow::showCountdown()
{
	_countdownLabel.show();
	_stackedLayout.setStackingMode(QStackedLayout::StackAll);
	_stackedLayout.setCurrentWidget(&_countdownLabel);
	_takePicButton->raise();
}

void PhotoboothWindow::hideCountdown()
{
	_countdownLabel.hide();
}

cv::Mat PhotoboothWindow::takePicture()
{
	QNetworkAccessManager* manager = new QNetworkAccessManager(this);

	QString req = _pCtx->getSettings()->settings().cameraURL;
	qDebug() << "Sending request : " << req;
	auto reply = manager->get(QNetworkRequest(QUrl(req)));

	QEventLoop loop;
	connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
	loop.exec();

	QByteArray bytes = reply->readAll();

	cv::Mat pic;
	if (!bytes.isEmpty())
	{
		qDebug() << "Received image !";
		cv::imdecode(std::vector<uchar>(bytes.begin(), bytes.end()), cv::IMREAD_COLOR, &pic);
	}
	else
	{
		qDebug() << "Empty response from camera";
		showInstuctions();
	}

	return pic;
}
