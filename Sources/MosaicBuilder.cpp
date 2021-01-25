#include "MosaicBuilder.h"
#include "QDir"

#include <QDebug>

using namespace cv;

MosaicBuilder::MosaicBuilder()
{
	_mosaicSize = 1000;
}

void MosaicBuilder::setBaseImage(cv::Mat image)
{
	_baseImage = image.clone();

	setMosaicSize(_mosaicSize);

	_meanColorsOfRois.clear();
	for (int i = 0; i < _mosaicSize; i++)
	{
		_meanColorsOfRois.insert(i, meanColorOfImage(_baseImage(getRoi(i))));
	}

	_mosaicImage = cv::Mat(_baseImage.rows, _baseImage.cols, CV_8UC3, Scalar(0));
}

void MosaicBuilder::setTilesDirectory(QString path)
{
	_tilesDirPath = path;
	_meanColorsOfTiles.clear();
	refreshTiles();
}

cv::Mat MosaicBuilder::refreshImage()
{
	clock_t c = clock();

	// Refresh tiles in case new images are in folder
	refreshTiles();

	qDebug() << "Refresh tiles : " << clock() - c;
	c = clock();

	// Build map matching between tiles and ROIs
	QMap<QString, QVector<int>> matches;
	QString bestTile;
	double bestDiff;
	for (int i = 0; i < _mosaicSize; i++)
	{
		 const QColor& roiMeanColor = _meanColorsOfRois[i];
		 bestTile = "";
		 bestDiff = -1;

		for (const auto& tile : _meanColorsOfTiles.keys())
		{
			const QColor& tileMeanColor = _meanColorsOfTiles[tile];
			double d = colorDiff(tileMeanColor, roiMeanColor);
			if (bestDiff < 0 || d < bestDiff)
			{
				bestTile = tile;
				bestDiff = d;
			}
		}

		matches[bestTile].push_back(i);
	}

	qDebug() << "Build map : " << clock() - c;
	c = clock();

	// Go through all images...
	for (const QString& imgName : matches.keys())
	{
		cv::Mat tileImage;
		cv::Mat miniTile;

		// ... and look for the tiles where it is suppose to go ...
		for (int index : matches[imgName])
		{
			// ... do something only if it is not the image already present
			if (_roiTileMap[index] != imgName)
			{		
				// ... and read image only once...
				if (tileImage.rows == 0)
				{
					tileImage = cv::imread(imgName.toStdString());
				}
				
				// ... get the corresponding ROI of base image...
				cv::Rect roi = getRoi(index);

				// ... and resize the "mini-image" only if necessary...
				if (miniTile.cols != roi.width || miniTile.rows != roi.height)
				{
					cv::resize(tileImage, miniTile, cv::Size(roi.width, roi.height));
				}
				
				// ... finally copy the image and register this change in the map
				miniTile.copyTo(_mosaicImage(roi));
				_roiTileMap[index] = imgName;
			}
		}
	}

	qDebug() << "Build mosaic : " << clock() - c;
	c = clock();

	return _mosaicImage;
}

void MosaicBuilder::setMosaicSize(int sz)
{
	_mosaicSize = sz;
	_numberOfRois = int(sqrt(sz)) + 1;
	_mosaicSize = _numberOfRois * _numberOfRois;
	_roiW = int(_baseImage.cols / _numberOfRois);
	_roiH = int(_baseImage.rows / _numberOfRois);
}

void MosaicBuilder::refreshTiles()
{
	QDir dir(_tilesDirPath);

	for (auto file : dir.entryList(QDir::Files))
	{
		QString path = _tilesDirPath + "/" + file;
		if (!_meanColorsOfTiles.contains(path))
		{
			cv::Mat tileImg = cv::imread(path.toStdString());
			if (tileImg.data != NULL)
			{
				cv::Mat miniTile;
				cv::resize(tileImg, miniTile, cv::Size(_roiW, _roiH));
				_meanColorsOfTiles.insert(path, meanColorOfImage(miniTile));
			}

			qDebug() << "Processed " + path;
		}
	}
}

cv::Rect MosaicBuilder::getRoi(int index)
{
	int y = index / _numberOfRois;
	int x = index - y * _numberOfRois;

	y = std::min(y, _baseImage.rows - 2);
	x = std::min(x, _baseImage.cols - 2);

	int w = std::min(_roiW, _baseImage.cols - 1 - x * _roiW);
	int h = std::min(_roiH, _baseImage.rows - 1 - y * _roiH);

	return cv::Rect(x * _roiW, y * _roiH, w, h);
}

QColor MosaicBuilder::meanColorOfImage(cv::Mat img)
{
	int r = 0;
	int g = 0; 
	int b = 0;
	uchar* pixels = img.data;

	int nPix = img.rows * img.cols;
	for (int k = nPix; k--;)
	{
		r += *pixels;
		pixels++;

		g += *pixels;
		pixels++;

		b += *pixels;
		pixels++;
	}

	QColor out = QColor(r / nPix, g / nPix, b / nPix);

	return out;
}

double MosaicBuilder::colorDiff(QColor c1, QColor c2)
{
	double meanRed = 0.5 * (c1.red() + c2.red());

	double dr = c1.red() - c2.red();
	double dg = c1.green() - c2.green();
	double db = c1.blue() - c2.blue();

	return sqrt((2.0 + meanRed / 256) * dr * dr + 4.0 * dg * dg + (2.0 + (255.0 - meanRed) / 256) * db * db);
}
