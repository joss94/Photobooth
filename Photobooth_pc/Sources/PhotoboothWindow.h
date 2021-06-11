#pragma once

#include "QWidget"

#include <QStackedLayout>
#include "QBoxLayout"
#include "VideoInput.h"
#include "QPushButton"

#include <QTimer>

class PhotoboothContext;
class SettingsWindow;

class PhotoboothWindow : public QWidget
{
	Q_OBJECT

signals:

	void askedForPicSignal();

	void picTakenSignal(cv::Mat pic);

public:

	PhotoboothWindow(PhotoboothContext* ctx, QWidget* parent = nullptr);

	void clickShutterButton();

	void showImage(const QImage& image);

	void showWaitMessage(QString msg);

	void setReadyForPicture(bool ready);

	void hideWaitMessage();

	void showInstuctions();

	void resizeEvent(QResizeEvent* event) override;

	void keyPressEvent(QKeyEvent* event) override;

private:

	void showCountdown();

	void hideCountdown();

	cv::Mat takePicture();

private:

	SettingsWindow* _pSettingsWindow = nullptr;
	cv::Mat _testPic;

	PhotoboothContext* _pCtx = nullptr;

	QVBoxLayout _mainLayout;
	QStackedLayout _stackedLayout;
	
	QLabel _instructionsLabel;
	QLabel _countdownLabel;
	QLabel _waitLabel;
	QLabel _frozenImageLabel;

	QPushButton* _takePicButton = nullptr;
	QPushButton* _settingsButton = nullptr;

	QTimer _countdownTimer;
	int _countdownValue;

	bool _readyForPicture = true;
};