#include "windows.h"

#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

#include "PhotoboothWindow.h"
#include "PhotoboothContext.h"
#include "SettingsWindow.h"

#include <QDebug>
#include <QString>
#include <QEventLoop>
#include "qevent.h"
#include "PhotoboothSettings.h"

PhotoboothWindow::PhotoboothWindow(PhotoboothContext* ctx, QWidget* parent) : QWidget(parent)
{
	_pCtx = ctx;

	_pSettingsWindow = new SettingsWindow(_pCtx);

	_countdownTimer.setSingleShot(true);
	connect(&_countdownTimer, &QTimer::timeout, this, [&]()
	{
		_countdownValue--;
		if (_countdownValue > 0)
		{
			QString txt = _countdownValue == 1 ? "On bouge plus" : QString::number(_countdownValue - 1);
			_countdownLabel.setText(QString("<font color='white'>" + txt + "</font>"));
			showCountdown();
			_countdownTimer.start(1000);
		}
		else
		{	
			cv::Mat pic = takePicture();
			if (pic.rows > 0)
			{
				emit picTakenSignal(pic.clone());
			}
			hideCountdown();
		}
	});

	_instructionsLabel.setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	_instructionsLabel.setScaledContents(true);
	QPixmap p;
	p.load(":/icons/instructions.jpg");
	_instructionsLabel.setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	_instructionsLabel.setPixmap(p);
	_stackedLayout.addWidget(&_instructionsLabel);

	QFont font = _countdownLabel.font();
	font.setPointSize(152);
	font.setBold(true);
	_countdownLabel.setFont(font);
	_countdownLabel.setAlignment(Qt::AlignCenter);
	_stackedLayout.addWidget(&_countdownLabel);

	font = _waitLabel.font();
	font.setPointSize(120);
	font.setBold(true);
	_waitLabel.setFont(font);
	_waitLabel.setAlignment(Qt::AlignCenter);
	_stackedLayout.addWidget(&_waitLabel);

	_stackedLayout.addWidget(&_frozenImageLabel);
	_stackedLayout.setCurrentWidget(&_instructionsLabel);

	_mainLayout.addLayout(&_stackedLayout);

	setLayout(&_mainLayout);

	_takePicButton = new QPushButton(this);
	_takePicButton->setGeometry(50, 50, 100, 100);
	_takePicButton->setIcon(QIcon(QPixmap(":/icons/camera.png")));
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
		if (_readyForPicture)
		{
			_readyForPicture = false;
			_countdownValue = 5;
			_countdownTimer.start(1);
			emit askedForPicSignal();
		}
	});



	_settingsButton = new QPushButton(this);
	_settingsButton->setGeometry(50, 50, 60, 60);
	_settingsButton->setIcon(QIcon(QPixmap(":/icons/gear.png")));
	_settingsButton->setIconSize(QSize(40, 40));
	_settingsButton->show();
	_settingsButton->raise();

	_settingsButton->setStyleSheet("\
		QPushButton {\
			background-color: #fff;\
			border: 2px solid #555;\
			border-radius: 30px;\
			padding: 5px;\
		}\
		\
		QPushButton:hover{\
			background-color: #888;\
		}");

	connect(_settingsButton, &QPushButton::clicked, this, [&]()
		{
			_pSettingsWindow->show();
		});
}

void PhotoboothWindow::clickShutterButton()
{
	_takePicButton->click();
}

void PhotoboothWindow::showImage(const QImage& image)
{
	_frozenImageLabel.setScaledContents(true);
	_frozenImageLabel.setPixmap(QPixmap::fromImage(image).scaled(_frozenImageLabel.size()));
	_stackedLayout.setCurrentWidget(&_frozenImageLabel);
	if (_readyForPicture)
	{
		_takePicButton->raise();
	}
	_settingsButton->raise();
}

void PhotoboothWindow::showWaitMessage(QString msg)
{
	_waitLabel.setText(QString("<font color='white'>" + msg + "</font>"));
	_waitLabel.show();
	_stackedLayout.setStackingMode(QStackedLayout::StackAll);
	_stackedLayout.setCurrentWidget(&_waitLabel);
	if (_readyForPicture)
	{
		_takePicButton->raise();
	}
	_settingsButton->raise();
}

void PhotoboothWindow::hideWaitMessage()
{
	_waitLabel.hide();
}

void PhotoboothWindow::showInstuctions()
{
	_stackedLayout.setCurrentWidget(&_instructionsLabel);
	_takePicButton->raise();
	_settingsButton->raise();
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

	if (_settingsButton != nullptr)
	{
		_settingsButton->setGeometry(
			(event->size().width() - _settingsButton->width()) - 20,
			(event->size().height() - _settingsButton->height()) - 20,
			_settingsButton->width(),
			_settingsButton->height());
	}
}

void PhotoboothWindow::setReadyForPicture(bool ready)
{
	_readyForPicture = ready;
	_takePicButton->raise();
}

void PhotoboothWindow::keyPressEvent(QKeyEvent* event) {
	switch (event->key()) {
	case Qt::Key_Space:
		clickShutterButton();
		break;
	case Qt::Key_S:
		_pSettingsWindow->show();
		break;
	case Qt::Key_R:
		_pCtx->refreshMosaic();
		break;
	default:
		event->ignore();
		break;
	}
}

void PhotoboothWindow::showCountdown()
{
	_countdownLabel.show();
	_stackedLayout.setStackingMode(QStackedLayout::StackAll);
	_stackedLayout.setCurrentWidget(&_countdownLabel);
	if (_readyForPicture)
	{
		_takePicButton->raise();
	}
	_settingsButton->raise();
}

void PhotoboothWindow::hideCountdown()
{
	_countdownLabel.hide();
}

cv::Mat PhotoboothWindow::takePicture()
{
	QNetworkAccessManager* manager = new QNetworkAccessManager(this);

	QString req = _pCtx->getSettings()->settings().cameraURL;
	if (_pCtx->getSettings()->settings().autoIp)
	{
		std::array<char, 128> buffer;
		std::string result;
		std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen("arp -a", "r"), _pclose);
		if (!pipe) {
			throw std::runtime_error("popen() failed!");
		}
		while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
			result += buffer.data();
		}
		
		QString r = result.c_str();
		for (auto s : r.split("\n")) 
		{
			if (s.contains("dynamic"))
			{
				r = s;
				break;
			}
		}

		QRegularExpression rx("\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}");
		auto match = rx.match(r);
		QString ip = match.captured();

		std::cout << "Auto IP: " << ip.toStdString();

		req.replace(rx, ip);
	}

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
		_readyForPicture = true;
		showInstuctions();
	}

	return pic;
}
