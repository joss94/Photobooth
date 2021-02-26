// -----------------------------------------------------------------------------
//  Copyright (c) 2017-2020, IXBLUE.
// -----------------------------------------------------------------------------
//  N O T I C E
//
//  THIS MATERIAL IS CONSIDERED TO BE THE INTELLECTUAL PROPERTY OF IXBLUE.
// -----------------------------------------------------------------------------

#include "windows.h"

#include <QtCore/QCoreApplication>
#include <QApplication>
#include <QDebug>

#include "PhotoboothContext.h"

PhotoboothContext* ctx = 0;
Q_DECLARE_METATYPE(cv::Mat)

BOOL WINAPI ConsoleHandlerRoutine(DWORD dwCtrlType) 
{
	if (CTRL_CLOSE_EVENT == dwCtrlType) 
    {
		qDebug() << "Deleting context";
		delete ctx;
		return FALSE;
	}

	return FALSE;
}

int main(int argc, char *argv[]) 
{
	qRegisterMetaType<cv::Mat>();

    SetConsoleCtrlHandler(ConsoleHandlerRoutine, TRUE);

    QApplication b(argc, argv);

    ctx = new PhotoboothContext();
    
	return b.exec();
}