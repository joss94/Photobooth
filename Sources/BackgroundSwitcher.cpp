#include "BackgroundSwitcher.h"
#include "vector"
#include "QLabel"

using namespace cv;

BackgroundSwitcher::BackgroundSwitcher()
{
	_net = dnn::readNetFromTensorflow(
		"c:/Dev/Photobooth/Resources/mask-rcnn-coco/frozen_inference_graph.pb",
		"c:/Dev/Photobooth/Resources/mask-rcnn-coco/mask_rcnn_inception_v2_coco_2018_01_28.pbtxt"
	);

	//cv::Mat dummy(600, 500, CV_8UC3, cv::Scalar(0));
	//switchBackground(dummy, dummy);
}

void BackgroundSwitcher::switchBackground(Mat original, Mat background)
{
	auto out = original.clone();

	// Create markers, all background at first
	Mat markers = Mat(out.rows, out.cols, CV_32S);
	markers = Scalar::all(0);

	// Find persons with Mask RCNN
	auto blob = dnn::blobFromImage(out, 1.0, cv::Size(out.cols, out.rows), cv::Scalar(), true);
	_net.setInput(blob);

	std::vector<cv::Mat> r;
	std::vector<cv::String> outLayers = { "detection_out_final", "detection_masks" };

	_net.forward(r, outLayers);

	// Fill markers based on Mask RCNN results
	postprocess(markers, r);

	// Apply watershed for better contours
	watershed(out, markers);

	// Replace non person pixels by new background
	for (int row = 0; row < markers.rows; row++)
	{
		for (int col = 0; col < markers.cols; col++)
		{
			if (markers.at<int>(row, col) != 2)
			{
				out.at<Vec3b>(row, col) = background.at<Vec3b>(row, col);
			}
		}
	}

	_processedImage = out.clone();
	emit backgroundSwitched();
}

// For each frame, extract the bounding box and mask for each detected object
void BackgroundSwitcher::postprocess(cv::Mat& markers, const std::vector<cv::Mat>& outs)
{
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

			int left = static_cast<int>(markers.cols * outDetections.at<float>(i, 3));
			int top = static_cast<int>(markers.rows * outDetections.at<float>(i, 4));
			int right = static_cast<int>(markers.cols * outDetections.at<float>(i, 5));
			int bottom = static_cast<int>(markers.rows * outDetections.at<float>(i, 6));
			left = max(0, min(left, markers.cols - 1));
			top = max(0, min(top, markers.rows - 1));
			right = max(0, min(right, markers.cols - 1));
			bottom = max(0, min(bottom, markers.rows - 1));
			Rect box = Rect(left, top, right - left + 1, bottom - top + 1);

			// Extract the mask for the object
			Mat objectMask(outMasks.size[2], outMasks.size[3], CV_32F, outMasks.ptr<float>(i, classId));

			// Resize the mask, threshold, color and apply it on the image
			resize(objectMask, objectMask, Size(box.width, box.height));

			for (int r = top; r < bottom; r++)
			{
				for (int c = left; c < right; c++)
				{
					float conf = objectMask.at<float>(r - top, c - left);
					int& marker = markers.at<int>(r, c);
					if (marker == 0)
					{
						marker = conf > 0.9 ? ((classId == 0) ? 2 : 3) : (conf < 0.1 ? 3 : 0);
					}
				}
			}
		}
	}
}
