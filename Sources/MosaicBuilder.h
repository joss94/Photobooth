#pragma once

#include "opencv2/opencv.hpp"
#include "QObject"
#include "QMap"
#include "QString"
#include "QColor"

class MosaicBuilder : public QObject
{
	Q_OBJECT;

public:

	MosaicBuilder();

	void setBaseImage(cv::Mat image);

	void setTilesDirectory(QString path);

	cv::Mat refreshImage();

	void setMosaicSize(int sz);

private:

	void refreshTiles();

	cv::Rect getRoi(int index);

	QColor meanColorOfImage(cv::Mat img);

	double colorDiff(QColor c1, QColor c2);

private:

	int _mosaicSize;
	int _numberOfRois;
	int _roiW;
	int _roiH;

	cv::Mat _baseImage;
	cv::Mat _mosaicImage;

	QString _tilesDirPath;

	QMap<QString, QColor> _meanColorsOfTiles;
	QMap<int, QColor> _meanColorsOfRois;
	QMap<int, QString> _roiTileMap;
};