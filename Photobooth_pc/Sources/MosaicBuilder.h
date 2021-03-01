#pragma once

#include "opencv2/opencv.hpp"
#include "QObject"
#include "QMap"
#include "QString"
#include "QColor"

struct matching_scores
{
	QString tileName;
	int index;
	double score;
};

class MosaicBuilder : public QObject
{
	Q_OBJECT;

signals:

	void mosaicUpdatedSignal(cv::Mat mosaic);

public:

	MosaicBuilder();

	void setBaseImage(QString path);

	void setTilesDirectory(QString path);

	cv::Mat refreshImage();

	cv::Mat refreshImageThread();

	void setMosaicSize(int szW, int szH);

	void setMaxOccurence(int maxOccurence);

	void setBaseOpacity(double opacity);

private:

	void refreshTiles();

	cv::Rect getRoi(int index);

	double getMatchScore(cv::Mat image1, cv::Mat image2);

private:

	int _sizeW;
	int _sizeH;
	int _mosaicSize;
	int _roiW;
	int _roiH;

	int _maxOccurence;
	double _baseOpacity;

	cv::Mat _baseImage;
	cv::Mat _largeBaseImage;
	cv::Mat _mosaicImage;

	QString _tilesDirPath;

	QMap<QString, QColor> _meanColorsOfTiles;
	QVector<QString> _roiTileMap;

	QVector<matching_scores> _matches;

	QMap<QString, cv::Mat> _miniImages;
};