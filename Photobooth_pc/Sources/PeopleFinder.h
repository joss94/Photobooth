// -----------------------------------------------------------------------------
//  Copyright (c) 2017-2020, IXBLUE.
// -----------------------------------------------------------------------------
//  N O T I C E
//
//  THIS MATERIAL IS CONSIDERED TO BE THE INTELLECTUAL PROPERTY OF IXBLUE.
// -----------------------------------------------------------------------------

#pragma once

#include <iostream>
#include <opencv2/opencv.hpp>
#include <filesystem>
#include "QString"

class Model;
class Tensor;

/// @class HorizonFinder
///
/// @brief Detects a horizon on an image using CNN segmentation and CUDA accelerated methods for output processing
class PeopleFinder
{

public:

	/// @brief Constructor
	/// @param[in] const QString & filename Path to the CNN weights
	/// @param[in] float gpuReq Percentage of GPU to dedicate to this CNN (Tensorflow gives all available by default)
	PeopleFinder(const QString& filepath, float gpuReq);

	/// @brief Destructor
	~PeopleFinder();

	/// @brief Horizon detection function
	///
	/// From input image, the original image is normalized to feed the segmentation CNN. The output are probabilities
	/// for every pixel to be sky or earth/sea. These are, these go through a Sobel function to find the earth to sky transition areas,
	/// and normalized in order to get a total probability of 1 for every column (exactly one horizon point per azimuth, i.e. per column).
	/// Results are finally resized to match the original input shape and returned as a 1 layer gray scale image.
	///
	/// @param[in] const cv::Mat & img Image on which to detect the horizon
	/// @return cv::Mat Final detection
	cv::Mat findHorizon(const cv::Mat& img);

	/// @brief Overload to find the horizon on several images, see findHorizon for a single image for more info
	/// @param[in] const std::vector<cv::Mat> & img Images on which to detect the horizon
	/// @return std::vector<cv::Mat> Final detections
	std::vector<cv::Mat> findHorizon(const std::vector<cv::Mat>& img);

	cv::Mat prepImage(const cv::Mat& image);

private:

	int _imgW;
	int _imgH;
	int _nbOfClasses;
	int _nbOfChannels;

	Model* _pModel = nullptr;
	Tensor* _pInput = nullptr;
	Tensor* _pOutput = nullptr;

	int _batchSize;
	std::vector<float> _inputs;
};