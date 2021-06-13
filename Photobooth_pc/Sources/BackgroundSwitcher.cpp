#include "BackgroundSwitcher.h"

#include "QApplication"

#include "vector"
#include "QLabel"

#include <QDir>
#include <algorithm>

#include <iostream>
#include "QDateTime"
#include "QDebug.h"

#include "windows.h"

using namespace cv;

BackgroundSwitcher::BackgroundSwitcher(QString scriptPath, QString modelPath) : 
	_scriptPath(scriptPath), _modelPath(modelPath)
{
	// Start DNN
	connect(this, &BackgroundSwitcher::outputReceived, this, &BackgroundSwitcher::onOutputReceived);
	_dnnUtilityDir = QCoreApplication::applicationDirPath();

	QDir().mkpath(_dnnUtilityDir + "/input");
	QDir().mkpath(_dnnUtilityDir + "/output");

	QStringList arguments;
	arguments << "-m" << correctPath(_modelPath)
		<< "-i" << correctPath(_dnnUtilityDir) + "/input"
		<< "-o" << correctPath(_dnnUtilityDir) + "/output";

	_pythonProcess = new QProcess();
	_pythonProcess->start(correctPath(_scriptPath), arguments);

	// Start roulette timer
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
	if(_pythonProcess != nullptr)
	{
		_pythonProcess->kill();
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
		while (!outputFound && (clock() - c) < 40000) 
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

void BackgroundSwitcher::onOutputReceived(QString path)
{
	cv::Mat r = cv::imread(path.toStdString());
	r.convertTo(_markersForeground, CV_32FC3, 1.0 / 255);

	_markersBackground = Scalar::all(1.0) - _markersForeground;

	_original.convertTo(_foreground, CV_32FC3);
	multiply(_markersForeground, _foreground, _foreground);
	emit imageProcessed();
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

QString BackgroundSwitcher::correctPath(QString path)
{
	QStringList pieces = path.split("/");
	QString output = "";
	for (auto& p : pieces)
	{
		if (p.contains(" "))
		{
			output += QString("\"") + p + QString("\"");
		}
		else
		{
			output += p;
		}

		output += "/";
	}

	return output.left(output.size() - 1);
}
