#include "BackgroundSwitcher.h"
#include "vector"
#include "QLabel"

#include <QDir>
#include <algorithm>

using namespace cv;

BackgroundSwitcher::BackgroundSwitcher()
{
	_net = dnn::readNetFromTensorflow(
		"c:/Dev/Photobooth/Resources/mask-rcnn-coco/frozen_inference_graph.pb",
		"c:/Dev/Photobooth/Resources/mask-rcnn-coco/mask_rcnn_inception_v2_coco_2018_01_28.pbtxt"
	);

	_rouletteTimer.setSingleShot(true);
	connect(&_rouletteTimer, &QTimer::timeout, this, [&]()
	{
		if (clock() - _rouletteStartTime < 10000 && _rouletteTime < 2000)
		{
			clock_t c = clock();
			int index;
			do
			{
				index = rand() % _rouletteFiles.size();
			} while (index == _lastRouletteIndex);

			_lastRouletteIndex = index;
			cv::Mat b = cv::imread(_rouletteFiles[index].toStdString());
			switchBackground(b);

			_rouletteTime = 5.0 + 1.1 * _rouletteTime;
			_rouletteTimer.start(std::max(0, int(_rouletteTime - (clock() - c))));
		}
	});
}

void BackgroundSwitcher::processNewFrame(cv::Mat frame)
{
	_original = frame.clone();

	// Find persons with Mask RCNN
	auto blob = dnn::blobFromImage(frame, 1.0, cv::Size(frame.cols, frame.rows), cv::Scalar(), true);
	_net.setInput(blob);

	std::vector<cv::Mat> r;
	std::vector<cv::String> outLayers = { "detection_out_final", "detection_masks" };

	_net.forward(r, outLayers);

	// Fill markers based on Mask RCNN results
	postprocess(r);

	emit imageProcessed();
}

void BackgroundSwitcher::switchBackground(Mat background)
{
	// Resize background to fit image
	cv::Mat tmp;
	cv::resize(background, tmp, cv::Size(_original.cols, _original.rows));

	// Clone image for modification
	cv::Mat out = _original.clone();

	// Replace non person pixels by new background
	for (int row = 0; row < _markers.rows; row++)
	{
		for (int col = 0; col < _markers.cols; col++)
		{
			if (_markers.at<int>(row, col) != 2)
			{
				out.at<Vec3b>(row, col) = tmp.at<Vec3b>(row, col);
			}
		}
	}

	emit backgroundSwitched(cv::Mat(out));
}

void BackgroundSwitcher::switchBackgroundRoulette(QString backgroundsFolder)
{
	QDir dir(backgroundsFolder);
	_rouletteFiles.clear();
	for (auto file : dir.entryList(QDir::Files))
	{
		QString path = backgroundsFolder + "/" + file;
		cv::Mat b = cv::imread(path.toStdString());
		if (b.data != NULL)
		{
			_rouletteFiles.push_back(path);
		}
	}

	_lastRouletteIndex = -1;
	_rouletteTime = 1;
	_rouletteStartTime = clock();
	_rouletteTimer.start(_rouletteTime);
}

// For each frame, extract the bounding box and mask for each detected object
void BackgroundSwitcher::postprocess(const std::vector<cv::Mat>& outs)
{
	// Create markers, all background at first
	_markers = Mat(_original.rows, _original.cols, CV_32S, Scalar(0));

	Mat outDetections = outs[0];
	Mat outMasks = outs[1];

	// Output size of masks is NxCxHxW where
	// N - number of detected boxes
	// C - number of classes (excluding background)
	// HxW - segmentation shape
	const int numDetections = outDetections.size[2];
	const int numClasses = outMasks.size[1];

	outDetections = outDetections.reshape(1, outDetections.total() / 7);
	for (int i = 0; i < numDetections; ++i)
	{
		float score = outDetections.at<float>(i, 2);
		if (score > 0)
		{
			// Extract the bounding box
			int classId = static_cast<int>(outDetections.at<float>(i, 1));

			if (classId == 0)
			{
				int left = static_cast<int>(_markers.cols * outDetections.at<float>(i, 3));
				int top = static_cast<int>(_markers.rows * outDetections.at<float>(i, 4));
				int right = static_cast<int>(_markers.cols * outDetections.at<float>(i, 5));
				int bottom = static_cast<int>(_markers.rows * outDetections.at<float>(i, 6));
				left = max(0, min(left, _markers.cols - 1));
				top = max(0, min(top, _markers.rows - 1));
				right = max(0, min(right, _markers.cols - 1));
				bottom = max(0, min(bottom, _markers.rows - 1));
				Rect box = Rect(left, top, right - left + 1, bottom - top + 1);

				// Extract the mask for the object
				Mat objectMask(outMasks.size[2], outMasks.size[3], CV_32F, outMasks.ptr<float>(i, classId));

				// Resize the mask and fill markers
				resize(objectMask, objectMask, Size(box.width, box.height));

				for (int r = top; r < bottom; r++)
				{
					for (int c = left; c < right; c++)
					{
						float conf = objectMask.at<float>(r - top, c - left);
						int& marker = _markers.at<int>(r, c);
						if (marker == 0)
						{
							if (conf > 0.8)
							{
								marker = 2;
							}
							else if (conf < 0.7)
							{
								marker = 3;
							}
						}
					}
				}
			}			
		}
	}

	// Apply watershed for better contours
	watershed(_original, _markers);
}
