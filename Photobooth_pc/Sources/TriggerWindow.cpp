#include "windows.h"

#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>

#include "TriggerWindow.h"
#include "PhotoboothContext.h"
#include "SettingsWindow.h"

#include <QDebug>
#include <QString>
#include <QEventLoop>
#include "qevent.h"
#include "PhotoboothSettings.h"

TriggerWindow::TriggerWindow(PhotoboothContext* ctx, QWidget* parent) : QWidget(parent)
{
	_pCtx = ctx;

	_takePicButton = new QPushButton(this);
	_takePicButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	_takePicButton->setIcon(QIcon(QPixmap(":/icons/camera.png")));
	_takePicButton->setIconSize(QSize(70, 70));

	_takePicButton->setStyleSheet("\
		QPushButton {\
			background-color: #fff;\
			border: 2px solid #555;\
			border-radius: 50px;\
			padding: 5px;\
		}\
		\
		QPushButton:hover{\
			background-color: #888;\
		}");

	connect(_takePicButton, &QPushButton::clicked, this, [&]() 
	{ 
		_pCtx->takePicture();
	});

	_mainLayout.addWidget(_takePicButton);	
	setLayout(&_mainLayout);

}

void TriggerWindow::keyPressEvent(QKeyEvent* event) {
	switch (event->key()) {
	case Qt::Key_Space:
		_takePicButton->click();
		break;
	default:
		event->ignore();
		break;
	}
}