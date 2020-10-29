#include "windows.h"

#include "PhotoboothWindow.h"
#include "PhotoboothContext.h"

void PhotoboothWindow::onTakePicButtonClicked()
{
	new std::thread([&]() {
		Sleep(3000);
		_pCtx->onNewPicTaken(_videoInput.getPicture());
	});
}

PhotoboothWindow::PhotoboothWindow(PhotoboothContext* ctx, QWidget* parent) : QWidget(parent)
{
	_pCtx = ctx;

	_takePicButton.setText("Take picture");

	_videoInput.setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

	_mainLayout.addWidget(&_videoInput);
	_mainLayout.addWidget(&_takePicButton);
	setLayout(&_mainLayout);

	connect(&_takePicButton, &QPushButton::clicked, this, &PhotoboothWindow::onTakePicButtonClicked);
}