#include "windows.h"
#include "VideoInput.h"
#include <QDebug>
#include <QEventLoop>

void VideoInput::onNewImage()
{
	setPixmap(QPixmap::fromImage(QImage(_image.data, _image.cols, _image.rows, QImage::Format_BGR888)));
}

void VideoInput::onNetworkFinished(QNetworkReply* reply)
{
}

VideoInput::VideoInput(QWidget* parent) : QLabel(parent)
{
	_capture = cv::VideoCapture(0, cv::CAP_DSHOW);

	int fourcc = _capture.get(cv::CAP_PROP_FOURCC);
	QString fourccStr = "";
	for (int i = 0; i < 4; i++)
	{
		fourccStr += char(fourcc >> 8 * i);
	}
	qDebug() << "FOURCC : " << fourccStr;

	startCaptureThread();

	connect(this, &VideoInput::newImage, this, &VideoInput::onNewImage);
}

cv::Mat VideoInput::getPicture()
{
	_waitingPicture = true;
	QNetworkAccessManager* manager = new QNetworkAccessManager(this);

	qDebug() << "Sending request : http://192.168.0.12:5000/takepicture";
	auto reply = manager->get(QNetworkRequest(QUrl("http://192.168.0.12:5000/takepicture")));
	
	QEventLoop loop;
	connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
	loop.exec();

	qDebug() << "Received reply";

	QByteArray bytes = reply->readAll();

	auto pixmap = QPixmap();
	cv::imdecode(std::vector<uchar>(bytes.begin(), bytes.end()), cv::IMREAD_COLOR, &_lastCapture);

	return _lastCapture.clone();
}

void VideoInput::startCaptureThread()
{
	if (!_catpuring)
	{
		_pCatpureThread = new std::thread(&VideoInput::captureThread, this);
	}
}

void VideoInput::captureThread()
{
	_catpuring = true;
	while (_catpuring)
	{
		if (isVisible())
		{
			//cv::Mat temp;
			if (_capture.read(_image))
			{
				//cv::resize(temp, _image, cv::Size(1920, 1080));
				emit newImage();
			}
			else
			{
				qDebug() << "Could not read frame";
			}
		}
		Sleep(30);
	}
}
