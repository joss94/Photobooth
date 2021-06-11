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
}

void MosaicBuilder::setBaseImage(QString image)
{
	cv::Mat original = imread(image.toStdString());
	cv::resize(original, _baseImage, cv::Size(1920 * 4, 1080 * 4));

	setMosaicSize(1.7 * 86, 1.7 * 56);

	_mosaicImage = Mat(_baseImage.rows, _baseImage.cols, CV_8UC3, Scalar(0));
}

void MosaicBuilder::setTilesDirectory(QString path)
{
	QDir().mkpath(path + "/resized");

	_tilesDirPath = path;
	_meanColorsOfTiles.clear();
	refreshTiles();
}


void MosaicBuilder::setMosaicSize(int szW, int szH)
{
	_sizeW = szW;
	_sizeH = szH;
	_mosaicSize = _sizeW * _sizeH;
	_roiW = int(_baseImage.cols / _sizeW) + 1;
	_roiH = int(_baseImage.rows / _sizeH) + 1;

	_roiTileMap.resize(_mosaicSize);
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

	// Build map matching between tiles and ROIs
	QVector<QString> tileMap;
	tileMap.resize(_mosaicSize);

	std::sort(_matches.begin(), _matches.end(), [](const matching_scores& a, const matching_scores& b) -> bool
	{
		return a.score < b.score;
	});

	_mosaicImage = Mat(_baseImage.rows, _baseImage.cols, CV_8UC3, Scalar(0));

	QVector<int> filledIndexes;
	QMap<QString, int> occurences;

	int refreshPreview = 0;

	// Filling with radius 3
	for (auto m : _matches)
	{
		if (!filledIndexes.contains(m.index))
		{
			bool usable = true;
			int radius = 3;

			// Limit on occurence
			if (_maxOccurence > 0 && occurences[m.tileName] > _maxOccurence)
			{
				usable = false;
			}

			// One image cannot be repeated in a 3-index radius
			if (usable)
			{
				for (int a = -radius; a <= radius; a++)
				{
					for (int b = -radius; b <= radius; b++)
					{
						int index = min(max(0, m.index + a + b * _sizeW), _mosaicSize - 1);
						if (tileMap[index] == m.tileName)
						{
							usable = false;
						}
					}
				}
			}

			if (usable)
			{
				// Set this new tile mapping
				tileMap[m.index] = m.tileName;

				// Set this index as filled
				filledIndexes.push_back(m.index);

				// Register tile in occurences table
				int o = occurences.value(m.tileName, 0);
				occurences[m.tileName] = o+1;

				// ... get the corresponding ROI of base image...
				Rect roi = getRoi(m.index);

				// ... and resize the "mini-image" only if necessary...
				cv::Mat miniTile = _miniImages[m.tileName];
				if (miniTile.cols != roi.width || miniTile.rows != roi.height)
				{
					Mat tmp;
					cv::resize(miniTile, tmp, cv::Size(roi.width, roi.height));
					miniTile = tmp;
				}

				// ... finally copy the image
				addWeighted(_baseImage(roi), _baseOpacity, miniTile, 1.0 - _baseOpacity, 0.0, _mosaicImage(roi));

				// Refresh preview every 10 tiles to avoid taking too much time
				refreshPreview = (refreshPreview + +1) % 10;
				if (refreshPreview == 0)
				{
					// Need to resize mosaic because if it is too large it will make UI thread lag
					cv::Mat resizedMosaic;
					cv::resize(_mosaicImage, resizedMosaic, cv::Size(1920, 1080));
					emit mosaicUpdatedSignal(resizedMosaic.clone());
					Sleep(10);
				}
			}
		}

		if (filledIndexes.size() == _mosaicSize)
		{
			break;
		}
	}

	qDebug() << "Build map : " << clock() - c;
	c = clock();

	// Save map for future changes
	_roiTileMap = tileMap;

	qDebug() << "Build mosaic : " << clock() - c;
	c = clock();

	cv::imwrite("test_mosaic.jpg", _mosaicImage);
	return _mosaicImage;
}

void MosaicBuilder::refreshTiles()
{
	QDir dir(_tilesDirPath);

	int timeRead = 0;
	int timeResize = 0;
	int timeDiff = 0;

	for (auto file : dir.entryList(QDir::Files))
	{
		QString path = _tilesDirPath + "/" + file;

		QString pathResized = _tilesDirPath + "/resized/" + file;
		if (!QFile::exists(pathResized))
		{
			Mat img = cv::imread(path.toStdString());
			Mat imgResized;
			cv::resize(img, imgResized, cv::Size(128, 128));
			imwrite(pathResized.toStdString(), imgResized);
		}

		path = pathResized;
		
		if (!_miniImages.contains(path))
		{
			clock_t c = clock();
			Mat tileImg = cv::imread(path.toStdString());
			timeRead += clock() - c;

			if (tileImg.data != NULL)
			{
				QVector<double> scores;
				for (int i = 0; i < _mosaicSize; i++)
				{
					c = clock();
					Mat baseTile = _baseImage(getRoi(i));

					Mat miniTile;
					resize(tileImg, miniTile, cv::Size(_roiW, _roiH));

					Mat miniTileBlurred;
					cv::blur(miniTile, miniTileBlurred, cv::Size(2, 2));

					_miniImages[path] = miniTileBlurred;
					timeResize += clock() - c;

					c = clock();
					matching_scores m;
					m.tileName = path;
					m.index = i;
					// Important to clone base tile here, because pointer increment is wrong
					// if we get a sub-ROI from a bigger image
					m.score = getMatchScore(baseTile.clone(), _miniImages[path]);;
					_matches << m;
					timeDiff += clock() - c;
				}
			}

			qDebug() << "Processed " + path;
		}
	}

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

double MosaicBuilder::getMatchScore(cv::Mat image1, cv::Mat image2)
{
	if (image2.cols != image1.cols || image2.rows != image1.rows)
	{
		Mat tmp;
		resize(image2, tmp, cv::Size(image1.cols, image1.rows));
		image2 = tmp;
	}

	double diff = 0;

	uchar* imagePixels = image2.data;
	uchar* basePixels = image1.data;

	int N = image1.rows * image1.cols;
	double Ninv = 1.0 / N;

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

	return diff * Ninv;
}