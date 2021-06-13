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

	void showMessageSignal(QString msg);

public:

	PhotoboothWindow(PhotoboothContext* ctx, QWidget* parent = nullptr);

	void clickShutterButton();

	void showImage(const QImage& image);

	void showWaitMessage(QString msg);

	void setReadyForPicture(bool ready);

	void hideWaitMessage();

	void keyPressEvent(QKeyEvent* event) override;

	void closeEvent(QCloseEvent* event) override;

private:

	cv::Mat takePicture();

private:

	SettingsWindow* _pSettingsWindow = nullptr;
	cv::Mat _testPic;

	PhotoboothContext* _pCtx = nullptr;

	QVBoxLayout _mainLayout;
	QWidget _stackedTopWidget;
	QWidget _stackedBottomWidget;
	QStackedLayout _stackedMain;
	QStackedLayout _stackedTop;

	QLabel _waitLabel;
	QLabel _frozenImageLabel;

	QPushButton* _takePicButton = nullptr;
	QPushButton* _settingsButton = nullptr;

	QTimer _countdownTimer;
	int _countdownValue;

	bool _readyForPicture = true;
};