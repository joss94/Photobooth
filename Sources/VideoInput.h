#pragma once

#include "QLabel"
#include "opencv2/opencv.hpp"
#include <thread>

class VideoInput : public QLabel
{
	Q_OBJECT;

private slots:

	void onNewImage();

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
	
	bool _catpuring = false;
	std::thread* _pCatpureThread = nullptr;

};