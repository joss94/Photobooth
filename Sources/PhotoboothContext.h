#include "opencv2/opencv.hpp";
#include "QObject"

class PhotoboothWindow;
class BackgroundSwitcher;

class PhotoboothContext : public QObject
{
	Q_OBJECT;

public:

	PhotoboothContext();

	~PhotoboothContext();

	void onNewPicTaken(cv::Mat picture);

private:

	PhotoboothWindow* _pWindow = nullptr;
	BackgroundSwitcher* _pSwitcher = nullptr;
};