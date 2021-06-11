#include "BackgroundSwitcher.h"

#include "QApplication"

#include "vector"
#include "QLabel"

#include <QDir>
#include <algorithm>

#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include "QDateTime"
#include "QDebug.h"
#include "windows.h"

using namespace cv;

std::string executeCommand(const char* cmd) 
{
	std::cout << "Executing command : " << cmd << std::endl;

	std::array<char, 128> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
	if (!pipe) {
		throw std::runtime_error("popen() failed!");
	}
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
		result += buffer.data();
	}
	return result;

}

void BackgroundSwitcher::onOutputReceived(QString path)
{
	auto r = cv::imread(path.toStdString());

	// Fill markers based on Mask RCNN results
	postprocess(r);

	emit imageProcessed();
}

BackgroundSwitcher::BackgroundSwitcher(QString scriptPath, QString modelPath) : 
	_scriptPath(scriptPath), _modelPath(modelPath)
{
	//_peopleFinder = new PeopleFinder(modelPath, 0.3);
	//_peopleFinder->findHorizon(cv::Mat(1920, 1080, CV_8UC3, cv::Scalar(0)));

	connect(this, &BackgroundSwitcher::outputReceived, this, &BackgroundSwitcher::onOutputReceived);
	_dnnUtilityDir = QCoreApplication::applicationDirPath();

	QDir().mkpath(_dnnUtilityDir + "/input");
	QDir().mkpath(_dnnUtilityDir + "/output");

	startDNN();

	_rouletteTimer.setSingleShot(true);
	connect(&_rouletteTimer, &QTimer::timeout, this, [&]()
	{
		if (clock() - _rouletteStartTime < 10000 && _rouletteTime < 2000)
		{
			clock_t c = clock();

			// Finding index of next background
			int index = 0;
			if (_rouletteFiles.size() > 1)
			{
				do
				{
					index = rand() % _rouletteFiles.size();
				} while (index == _lastRouletteIndex);
				_lastRouletteIndex = index;
			}

			// If background does not have the right size, resize it and save it to avoid doing it again in the future
			cv::Mat b = cv::imread(_rouletteFiles[index].toStdString());
			if (b.cols != 1920 || b.rows != 1080)
			{
				Mat tmp;
				resize(b, tmp, cv::Size(1920, 1080));
				b = tmp;
				imwrite(_rouletteFiles[index].toStdString(), b);
			}

			// Replace the background
			_switched = switchBackground(b);

			// If only one background available, then stop right now, otherwise keep rolling
			if (_rouletteFiles.size() == 1)
			{
				emit rouletteFinished(_switched);
			}
			else
			{
				_rouletteTime = 1.2 * _rouletteTime;
				_rouletteTimer.start(std::max(30, int(_rouletteTime - (clock() - c))));
			}
			
		}
		else
		{
			emit rouletteFinished(_switched);
		}
	});
}

BackgroundSwitcher::~BackgroundSwitcher()
{
	if (_pDnnThread != nullptr)
	{
		delete _pDnnThread;
	}
}

void BackgroundSwitcher::processNewFrame(cv::Mat frame)
{
	// Background switcher only works with 1920x1080 images for optimization purpose
	resize(frame, _original, cv::Size(1920, 1080));

	// Copy image to input dirs, python script will detecet it automatically
	QString inputName = QDateTime::currentDateTime().toString("yyyy_MM_dd_hh_mm_ss");

	cv::imwrite((_dnnUtilityDir + "/input/" + inputName + ".jpg").toStdString(), _original);
	new std::thread([&](QString name) 
	{
		QString outputFile = _dnnUtilityDir + "/output/" + name + ".jpg";
		clock_t c = clock();

		bool outputFound = false;
		// Wait for output file to appear, with 15s timeout
		while (!outputFound && (clock() - c) < 15000) 
		{
			outputFound = QFile::exists(outputFile);
			Sleep(20);
		}

		if (outputFound)
		{
			Sleep(1000);
			qDebug() << "New file received : " << outputFile;
			emit outputReceived(outputFile);
		}
		else
		{
			qDebug() << "Never found output";
		}
	}, inputName);
}

cv::Mat BackgroundSwitcher::switchBackground(Mat newBackground)
{
	cv::Mat background;
	newBackground.convertTo(background, CV_32FC3);

	// Storage for output image
	cv::Mat out;
	multiply(_markersBackground, background, background);
	add(_foreground, background, out);
	
	out.convertTo(out, CV_8UC3);

	emit backgroundSwitched(out.clone());

	return out;
}

void BackgroundSwitcher::switchBackgroundRoulette(QString backgroundsFolder)
{
	QDir dir(backgroundsFolder);
	_rouletteFiles.clear();
	QStringList filters;
	filters << "*.jpg" << "*.png" << "*.jpeg";
	QStringList files = dir.entryList(filters, QDir::Files);

	for (auto file : files)
	{
		QString path = backgroundsFolder + "/" + file;
		cv::Mat b = cv::imread(path.toStdString());
		if (b.data != NULL)
		{
			_rouletteFiles.push_back(path);
		}
	}

	if (_rouletteFiles.size() > 0)
	{
		_lastRouletteIndex = -1;
		_rouletteTime = 30;
		srand(time(NULL));
		_rouletteStartTime = clock();
		_rouletteTimer.start(_rouletteTime);
	}
	else
	{
		rouletteFinished(_original.clone());
	}
}

// For each frame, extract the bounding box and mask for each detected object
void BackgroundSwitcher::postprocess(const cv::Mat& out)
{
	out.convertTo(_markersForeground, CV_32FC3, 1.0 / 255);

	_markersBackground = Scalar::all(1.0) - _markersForeground;		

	_original.convertTo(_foreground, CV_32FC3);
	multiply(_markersForeground, _foreground, _foreground);
}

void BackgroundSwitcher::startDNN()
{	
	_pDnnThread = new std::thread([&]() {
		executeCommand(QString("%1 -m %2 -i %3 -o %4")
			.arg(_scriptPath)
			.arg(_modelPath)
			.arg(_dnnUtilityDir + "/input")
			.arg(_dnnUtilityDir + "/output").toStdString().c_str());
	});
}
