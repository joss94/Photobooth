#pragma once

#include "QLabel"
#include "opencv2/opencv.hpp"
#include <thread>

#include <QNetworkAccessManager>
#include <QNetworkReply>

class VideoInput : public QLabel
{
	Q_OBJECT;

private slots:

	void onNewImage();

	void onNetworkFinished(QNetworkReply* reply);

signals:

	void newImage();

public:

	VideoInput(QWidget* parent = NULL);

	cv::Mat getPicture();

private:

	void startCaptureThread();

	void captureThread();

private:

	cv::VideoCapture _capture;
	cv::Mat _image;
	cv::Mat _lastCapture;

	bool _catpuring = false;
	bool _waitingPicture = false;
	std::thread* _pCatpureThread = nullptr;

};