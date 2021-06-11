#pragma once

#include "QWidget"

#include <QStackedLayout>
#include "QBoxLayout"
#include "VideoInput.h"
#include "QPushButton"

#include <QTimer>

class PhotoboothContext;
class SettingsWindow;

class TriggerWindow : public QWidget
{
	Q_OBJECT

public:

	TriggerWindow(PhotoboothContext* ctx, QWidget* parent = nullptr);

	void keyPressEvent(QKeyEvent* event) override;

private:

	PhotoboothContext* _pCtx = nullptr;

	QVBoxLayout _mainLayout;

	QPushButton* _takePicButton = nullptr;

	bool _readyForPicture = true;
};