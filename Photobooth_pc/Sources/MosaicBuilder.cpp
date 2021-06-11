#include "MosaicBuilder.h"
#include "QDir"

#include <QDebug>
#include <windows.h>

using namespace cv;
using namespace std;

MosaicBuilder::MosaicBuilder()
{
	_maxOccurence = -1;
	_baseOpacity = 0.3;
	_displaySpeed = 10;
}

void MosaicBuilder::setBaseImage(QString image)
{
	cv::Mat original = imread(image.toStdString());
	cv::resize(original, _baseImage, cv::Size(1920 * 1.0, 1080 * 1.0));

	setMosaicSize(_sizeW, _sizeH);
	_mosaicImage = Mat(_baseImage.rows, _baseImage.cols, CV_8UC3, Scalar(0));
}

void MosaicBuilder::setTilesDirectory(QString path)
{
	std::cout << "Setting tiles directory: " << path.toStdString();
	QDir().mkpath(path + "/resized");
	_tilesDirPath = path;
	refreshTiles();
}

void MosaicBuilder::setMosaicSize(int szW, int szH)
{
	bool different = szW != _sizeW || szH != _sizeH;

	_sizeW = szW;
	_sizeH = szH;
	_mosaicSize = _sizeW * _sizeH;
	_roiW = int(_baseImage.cols / _sizeW) + 1;
	_roiH = int(_baseImage.rows / _sizeH) + 1;

	_miniImages.clear();
	_matches.clear();

	if (_tilesDirPath != "")
	{
		setTilesDirectory(_tilesDirPath);
	}
}

void MosaicBuilder::setMaxOccurence(int maxOccurence)
{
	_maxOccurence = maxOccurence;
}

void MosaicBuilder::setBaseOpacity(double opacity)
{
	if (opacity >= 0.0 && opacity <= 1.0)
	{
		_baseOpacity = opacity;
	}
}

void MosaicBuilder::setDisplaySpeed(int speed)
{
	_displaySpeed = speed;
}

cv::Mat MosaicBuilder::refreshImage()
{
	new std::thread(&MosaicBuilder::refreshImageThread, this);
	return _mosaicImage;
}

cv::Mat MosaicBuilder::refreshImageThread()
{
	clock_t c = clock();

	// Refresh tiles in case new images are in folder
	refreshTiles();

	qDebug() << "Refresh tiles : " << clock() - c;
	c = clock();

	std::sort(_matches.begin(), _matches.end(), [](const matching_scores& a, const matching_scores& b) -> bool
	{
		return a.score < b.score;
	});

	_mosaicImage = Mat(_baseImage.rows, _baseImage.cols, CV_8UC3, Scalar(0));

	// Build map matching between tiles and ROIs
	QVector<QString> tileMap(_mosaicSize, "");
	QVector<bool> filledIndexes(_mosaicSize, false);
	QMap<QString, int> occurences;
	int refreshPreview = 0;

	QVector<matching_scores> matches(_matches);
	for (int radius = 3; radius >= 0; radius--)
	{
		int time1 = 0;
		int time2 = 0;
		int time3 = 0;
	
		QVector<matching_scores> nextMatches;
		std::cout << "Building with radius " << radius << std::endl;
	
		for (int matchId = 0; matchId < matches.size(); matchId++)
		{
			auto c = clock();
			auto& m = _matches[matchId];
			auto& tileName = m.tileName;
			auto& index = m.index;

			bool usable = true;
			bool discardedBecauseOfRadius = false;
			int o = occurences.value(tileName, 0);

			// Do not use if index is already taken, and tile already appears somewhere else
			if(o > 0 && filledIndexes[index])
			{
				usable = false;
			}
			time1 += clock() - c;

			// Limit on occurence
			c = clock();
			if (usable && _maxOccurence > 0 && o > _maxOccurence)
			{
				usable = false;
			}
			time2 += clock() - c;

			// Limit on radius
			c = clock();
			if (usable && radius > 0)
			{
				for (int a = -radius; a <= radius; a++)
				{
					for (int b = -radius; b <= radius; b++)
					{
						int idx = index + a + b * _sizeW;
						usable = idx < 0 || idx >= _mosaicSize || !filledIndexes[idx] || tileMap[idx] != tileName;

						if (!usable)
						{
							discardedBecauseOfRadius = true;
							break;
						}
					}

					if (!usable)
					{
						break;
					}
				}
			}
			time3 += clock() - c;

			// If image is usable, then place it in mosaic
			if (usable)
			{
				// Set this new tile mapping
				tileMap[index] = tileName;

				// Set this index as filled
				filledIndexes[index] = true;

				// Register tile in occurences table
				occurences[tileName] = o + 1;

				// ... get the corresponding ROI of base image...
				Rect roi = getRoi(index);

				// ... and resize the "mini-image" only if necessary...
				cv::Mat miniTile = _miniImages[tileName];
				if (miniTile.cols != roi.width || miniTile.rows != roi.height)
				{
					miniTile = miniTile(cv::Rect(0, 0, roi.width, roi.height));
				}

				// ... finally copy the image
				addWeighted(_baseImage(roi), _baseOpacity, miniTile, 1.0 - _baseOpacity, 0.0, _mosaicImage(roi));

				// Refresh preview every 10 tiles to avoid taking too much time
				if (refreshPreview == 0)
				{
					// Need to resize mosaic because if it is too large it will make UI thread lag
					cv::Mat resizedMosaic;
					cv::resize(_mosaicImage, resizedMosaic, cv::Size(1920, 1080));
					emit mosaicUpdatedSignal(resizedMosaic.clone(), false);
					Sleep(10);
				}
				refreshPreview = (refreshPreview + 1) % _displaySpeed;	
			}
			else if (true || discardedBecauseOfRadius)
			{
				nextMatches.push_back(m);
			}
		}

		matches = QVector<matching_scores>(nextMatches);
	}

	cv::Mat resizedMosaic;
	cv::resize(_mosaicImage, resizedMosaic, cv::Size(1920, 1080));
	emit mosaicUpdatedSignal(resizedMosaic.clone(), true);
	Sleep(10);

	qDebug() << "Build map : " << clock() - c;
	c = clock();

	qDebug() << "Build mosaic : " << clock() - c;
	c = clock();

	cv::imwrite("test_mosaic.jpg", _mosaicImage);
	return _mosaicImage;
}

void MosaicBuilder::refreshTiles()
{
	if (!QDir().exists(_tilesDirPath))
	{
		std::cout << "Tiles directory does not exist : " << _tilesDirPath.toStdString() << std::endl;
		return;
	}

	QDir dir(_tilesDirPath);

	int timeRead = 0;
	int timeResize = 0;
	int timeDiff = 0;

	// Adapt mosaic size based on number of tiles
	int nTiles = dir.entryList(QDir::Files).size();
	int sizeW = sqrt((1920.0 / 1080.0) * nTiles * 35) + 1;
	int sizeH = /*(1080.0 / 1920) **/ sizeW;
	if (sizeW != _sizeW || sizeH != _sizeH)
	{
		setMosaicSize(sizeW, sizeH);
		return;
	}

	for (auto file : dir.entryList(QDir::Files))
	{
		QString path = _tilesDirPath + "/" + file;

		QString pathResized = _tilesDirPath + "/resized/" + file;
		if (!QFile::exists(pathResized))
		{
			Mat img = cv::imread(path.toStdString());
			Mat imgResized;
			cv::resize(img, imgResized, cv::Size(256, 256));
			imwrite(pathResized.toStdString(), img);
		}

		path = pathResized;
		
		if (!_miniImages.contains(path))
		{
			clock_t c = clock();
			Mat tileImg = cv::imread(path.toStdString());
			timeRead += clock() - c;

			if (tileImg.data != NULL)
			{
				Mat miniTile;
				cv::resize(tileImg, miniTile, cv::Size(_roiW, _roiH));

				_miniImages[path] = miniTile;

				for (int i = 0; i < _mosaicSize; i++)
				{
					c = clock();
					// Important to clone base tile here, because pointer increment is wrong
					// if we get a sub-ROI from a bigger image
					Mat baseTile = _baseImage(getRoi(i)).clone();

					timeResize += clock() - c;

					c = clock();
					matching_scores m;
					m.tileName = path;
					m.index = i;

					m.score = getMatchScore(baseTile, miniTile);;
					_matches << m;
					timeDiff += clock() - c;
				}
			}

			qDebug() << "Processed " + path;
		}
	}

	_maxOccurence = 3 * (_mosaicSize / _miniImages.size());

	qDebug() << "Time read : " << timeRead;
	qDebug() << "Time resize : " << timeResize;
	qDebug() << "Time diff : " << timeDiff;
}

cv::Rect MosaicBuilder::getRoi(int index)
{
	int y = index / _sizeW;
	int x = index - y * _sizeW;

	y = min(y * _roiH, _baseImage.rows - 2);
	x = min(x * _roiW, _baseImage.cols - 2);

	int w = min(_roiW, _baseImage.cols - 1 - x);
	int h = min(_roiH, _baseImage.rows - 1 - y);

	return Rect(x, y, w, h);
}

double MosaicBuilder::getMatchScore(cv::Mat& image1, cv::Mat& image2)
{
	double diff = 0;

	uchar* imagePixels = image2.data;
	uchar* basePixels = image1.data;

	int N = image1.rows * image1.cols;

	double diffR, diffG, diffB;

	for (int i = image1.rows * image1.cols; i--;)
	{
		diffR = (*imagePixels - *basePixels);

		imagePixels++;
		basePixels++;

		diffG = (*imagePixels - *basePixels);

		imagePixels++;
		basePixels++;

		diffB = (*imagePixels - *basePixels);

		imagePixels++;
		basePixels++;

		diff += sqrt(diffR * diffR + diffG * diffG + diffB * diffB);
	}

	return diff / N;
}