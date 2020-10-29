#include "QWidget"

#include "QBoxLayout"
#include "VideoInput.h"
#include "QPushButton"

class PhotoboothContext;

class PhotoboothWindow : public QWidget
{

private slots:

	void onTakePicButtonClicked();

public:

	PhotoboothWindow(PhotoboothContext* ctx, QWidget* parent = NULL);

private:

	PhotoboothContext* _pCtx = nullptr;

	QVBoxLayout _mainLayout;

	VideoInput _videoInput;
	QPushButton _takePicButton;

};