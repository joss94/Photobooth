#pragma once

#include "opencv2/opencv.hpp"
#include <thread>
#include "QObject"
#include "QTimer"
#include "QVector"
#include <time.h>

class PeopleFinder;

class BackgroundSwitcher : public QObject
{
	Q_OBJECT;

signals:

	void imageProcessed();

	void backgroundSwitched(cv::Mat newImage);

	void rouletteFinished(cv::Mat finalImage);

	void outputReceived(QString path);

private slots:

	void onOutputReceived(QString path);

public:

	BackgroundSwitcher(QString scriptPath, QString modelPath);

	~BackgroundSwitcher();

	void processNewFrame(cv::Mat frame);

	cv::Mat switchBackground(cv::Mat background);

	void switchBackgroundRoulette(QString backgroundsFolder);

	void postprocess(const cv::Mat& out);

private:

	void startDNN();

private:

	PeopleFinder* _peopleFinder = nullptr;

	QString _dnnUtilityDir;
	QString _modelPath;
	QString _scriptPath;

	cv::Mat _original;
	cv::Mat _markersForeground;
	cv::Mat _markersBackground;
	cv::Mat _foreground;
	cv::Mat _switched;

	QTimer _rouletteTimer;
	QStringList _rouletteFiles;
	int _lastRouletteIndex;
	int _rouletteTime;
	clock_t _rouletteStartTime;

	std::thread* _pDnnThread = nullptr;
};