#include "windows.h"
#include "VideoInput.h"
#include <QDebug>

void VideoInput::onNewImage()
{
	setPixmap(QPixmap::fromImage(QImage(_image.data, _image.cols, _image.rows, QImage::Format_BGR888)));
}

VideoInput::VideoInput(QWidget* parent) : QLabel(parent)
{
	_capture = cv::VideoCapture(0);

	_capture.set(cv::CAP_PROP_FRAME_WIDTH, 1920);
	_capture.set(cv::CAP_PROP_FRAME_HEIGHT, 1080);

	startCaptureThread();

	connect(this, &VideoInput::newImage, this, &VideoInput::onNewImage);
}

cv::Mat VideoInput::getPicture()
{
	return _image.clone();
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
		cv::Mat temp;
		if (_capture.read(temp))
		{
			cv::resize(temp, _image, cv::Size(1920, 1080));
			emit newImage();
		}
		else
		{
			qDebug() << "Could not read frame";
		}
		Sleep(30);
	}
}
