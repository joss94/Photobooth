#pragma once

#include "opencv2/opencv.hpp"
#include <thread>
#include "QObject"
#include "QTimer"
#include "QVector"
#include <time.h>

class BackgroundSwitcher : public QObject
{
	Q_OBJECT;

signals:

	void imageProcessed();

	void backgroundSwitched(cv::Mat newImage);

public:

	BackgroundSwitcher();

	void processNewFrame(cv::Mat frame);

	void switchBackground(cv::Mat background);

	void switchBackgroundRoulette(QString backgroundsFolder);

	void postprocess(const std::vector<cv::Mat>& outs);

private:

	cv::Mat _original;
	cv::Mat _markers;

	cv::dnn::dnn4_v20200609::Net _net;

	QTimer _rouletteTimer;
	QStringList _rouletteFiles;
	int _lastRouletteIndex;
	int _rouletteTime;
	clock_t _rouletteStartTime;
};