#include "opencv2/opencv.hpp"
#include <thread>
#include "QObject"

class BackgroundSwitcher : public QObject
{
	Q_OBJECT;

signals:

	void backgroundSwitched();

public:

	BackgroundSwitcher();

	void switchBackground(cv::Mat original, cv::Mat background);

	void postprocess(cv::Mat& markers, const std::vector<cv::Mat>& outs);

public:

	cv::Mat _processedImage;

private:

	cv::dnn::dnn4_v20200609::Net _net;
};