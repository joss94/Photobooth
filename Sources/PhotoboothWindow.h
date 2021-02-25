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

	void showInstuctions();

	void resizeEvent(QResizeEvent* event) override;

private:

	void showCountdown();

	void hideCountdown();

	cv::Mat takePicture();

private:

	cv::Mat _testPic;

	PhotoboothContext* _pCtx = nullptr;

	QVBoxLayout _mainLayout;
	QStackedLayout _stackedLayout;
	
	QLabel _instructionsLabel;
	QLabel _countdownLabel;
	QLabel _frozenImageLabel;

	QPushButton* _takePicButton = nullptr;

	QTimer _countdownTimer;
	int _countdownValue;

	bool _takingPicture = false;

};