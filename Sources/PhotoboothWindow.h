#pragma once

#include "QWidget"

#include <QStackedLayout>
#include "QBoxLayout"
#include "VideoInput.h"
#include "QPushButton"

#include <QTimer>

class PhotoboothContext;

class PhotoboothWindow : public QWidget
{
	Q_OBJECT

signals:

	void picTakenSignal(cv::Mat pic);

public:

	PhotoboothWindow(PhotoboothContext* ctx, QWidget* parent = nullptr);

	void showImage(const QImage& image);

	void showCamera();

private:

	void showCountdown();

private:

	PhotoboothContext* _pCtx = nullptr;

	QVBoxLayout _mainLayout;
	QStackedLayout _stackedLayout;

	VideoInput _videoInput;
	QLabel _countdownLabel;
	QLabel _frozenImageLabel;

	QHBoxLayout _buttonsLayout;
	QPushButton _takePicButton;
	QPushButton _showMosaicButton;

	QTimer _countdownTimer;
	int _countdownValue;

	bool _newPicture = false;

};