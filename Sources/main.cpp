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

BOOL WINAPI ConsoleHandlerRoutine(DWORD dwCtrlType) 
{
	if (CTRL_CLOSE_EVENT == dwCtrlType) 
    {
		delete ctx;
		return TRUE;
	}

	return FALSE;
}

int main(int argc, char *argv[]) 
{
    SetConsoleCtrlHandler(ConsoleHandlerRoutine, TRUE);

    QApplication b(argc, argv);

    ctx = new PhotoboothContext();
    
	return b.exec();
}