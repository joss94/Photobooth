#include "windows.h"

#include "PhotoboothWindow.h"
#include "PhotoboothContext.h"

#include <QDebug>

PhotoboothWindow::PhotoboothWindow(PhotoboothContext* ctx, QWidget* parent) : QWidget(parent)
{
	_pCtx = ctx;

	_countdownTimer.setSingleShot(true);
	connect(&_countdownTimer, &QTimer::timeout, this, [&]()
	{
		_countdownValue--;
		_countdownLabel.setText(QString("<font color='white'>" + QString::number(_countdownValue) + "</font>"));
		if (_countdownValue > 0)
		{
			_countdownLabel.show();
			_stackedLayout.setStackingMode(QStackedLayout::StackAll);
			_stackedLayout.setCurrentWidget(&_countdownLabel);
			_countdownTimer.start(1000);
		}
		else
		{
			_countdownLabel.hide();
			_stackedLayout.setCurrentWidget(&_videoInput);
			_stackedLayout.setStackingMode(QStackedLayout::StackOne);
			emit picTakenSignal(_videoInput.getPicture());
		}
	});

	_videoInput.setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	_stackedLayout.addWidget(&_videoInput);

	QFont font = _countdownLabel.font();
	font.setPointSize(152);
	font.setBold(true);
	_countdownLabel.setFont(font);
	_countdownLabel.setAlignment(Qt::AlignCenter);
	_stackedLayout.addWidget(&_countdownLabel);

	_stackedLayout.addWidget(&_frozenImageLabel);
	_stackedLayout.setCurrentWidget(&_videoInput);

	_mainLayout.addLayout(&_stackedLayout);

	_takePicButton.setText("Take picture");
	_buttonsLayout.addWidget(&_takePicButton);
	_showMosaicButton.setText("Show mosaic");
	_buttonsLayout.addWidget(&_showMosaicButton);
	_mainLayout.addLayout(&_buttonsLayout);

	setLayout(&_mainLayout);

	connect(&_takePicButton, &QPushButton::clicked, this, [&]() 
	{ 
		if (_newPicture == false)
		{
			showCountdown();
			_newPicture = true;
			_takePicButton.setText("Take a new picture");
		}
		else
		{
			showCamera();
			_newPicture = false;
			_takePicButton.setText("Take picture");
		}
	});

	connect(&_showMosaicButton, &QPushButton::clicked, _pCtx, &PhotoboothContext::onShowMosaicClicked);
}

void PhotoboothWindow::showImage(const QImage& image)
{
	_frozenImageLabel.setScaledContents(true);
	_frozenImageLabel.setPixmap(QPixmap::fromImage(image).scaled(_frozenImageLabel.size()));
	_stackedLayout.setCurrentWidget(&_frozenImageLabel);
}

void PhotoboothWindow::showCamera()
{
	_stackedLayout.setCurrentWidget(&_videoInput);
}

void PhotoboothWindow::showCountdown()
{
	_countdownValue = 4;
	_countdownTimer.start(1);
}
