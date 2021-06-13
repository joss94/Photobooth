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
			showWaitMessage(txt);
			_countdownTimer.start(1000);
		}
		else
		{	
			cv::Mat pic = takePicture();
			if (pic.rows > 0)
			{
				emit picTakenSignal(pic.clone());
			}
		}
	});

	_stackedTop.setStackingMode(QStackedLayout::StackAll);

	QFont font = _waitLabel.font();
	font.setPointSize(80);
	font.setBold(true);
	_waitLabel.setFont(font);
	_waitLabel.setAlignment(Qt::AlignCenter);
	_stackedTop.addWidget(&_waitLabel);

	QWidget* _buttonsWidget = new QWidget();
	QHBoxLayout* _buttonsLayout = new QHBoxLayout();
	_buttonsWidget->setLayout(_buttonsLayout);
	_stackedTop.addWidget(_buttonsWidget);
	_buttonsLayout->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);

	_takePicButton = new QPushButton(this);
	_takePicButton->setIcon(QIcon(QPixmap(":/icons/camera.png")));
	_takePicButton->setFixedSize(100, 100);
	_takePicButton->setIconSize(QSize(70, 70));
	_buttonsLayout->addWidget(_takePicButton);

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
		}"
	);

	connect(_takePicButton, &QPushButton::clicked, this, [&]() 
	{ 
		if (_readyForPicture)
		{
			setReadyForPicture(false);
			_countdownValue = 5;
			_countdownTimer.start(1);
			emit askedForPicSignal();
		}
	});

	_settingsButton = new QPushButton(this);
	_settingsButton->setIcon(QIcon(QPixmap(":/icons/gear.png")));
	_settingsButton->setFixedSize(60, 60);
	_settingsButton->setIconSize(QSize(40, 40));
	_buttonsLayout->addWidget(_settingsButton);

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
		}"
	);

	connect(_settingsButton, &QPushButton::clicked, this, [&]()
		{
			_pSettingsWindow->show();
		}
	);

	_frozenImageLabel.setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	_stackedMain.addWidget(&_frozenImageLabel);

	_stackedTopWidget.setLayout(&_stackedTop);
	_stackedMain.addWidget(&_stackedTopWidget);

	_stackedMain.setStackingMode(QStackedLayout::StackAll);
	_mainLayout.addLayout(&_stackedMain);

	setLayout(&_mainLayout);

	showImage(QImage(":/icons/instructions.jpg"));
}

void PhotoboothWindow::clickShutterButton()
{
	_takePicButton->click();
}

void PhotoboothWindow::showImage(const QImage& image)
{
	_frozenImageLabel.setScaledContents(true);
	_frozenImageLabel.setPixmap(QPixmap::fromImage(image));
}

void PhotoboothWindow::showWaitMessage(QString msg)
{
	_waitLabel.setText(QString("<font color='white'>" + msg + "</font>"));
	_stackedTop.setCurrentWidget(&_waitLabel);
	_stackedMain.setCurrentWidget(&_stackedTopWidget);
}

void PhotoboothWindow::hideWaitMessage()
{
	_waitLabel.setText("");
}

void PhotoboothWindow::setReadyForPicture(bool ready)
{
	_readyForPicture = ready;
	_takePicButton->setEnabled(ready);
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

void PhotoboothWindow::closeEvent(QCloseEvent* event)
{
	delete _pCtx;
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
			if (s.contains("dynam"))
			{
				r = s;
				break;
			}
		}

		QRegularExpression rx("\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}");
		auto match = rx.match(r);
		QString ip = match.captured();

		std::cout << "Auto IP: " << ip.toStdString() << std::endl;

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
		setReadyForPicture(true);
		showImage(QImage(":/icons/instructions.jpg"));
	}

	return pic;
}
