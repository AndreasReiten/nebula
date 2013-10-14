/* Copyright 2013 Andreas Reiten
 * This file is part of Nebula.
 *
 * Nebula is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation (version 3).
 *
 * Nebula is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Nebula.  If not, see <http://www.gnu.org/licenses/>. */

#include <QCoreApplication>
#include <QApplication>
#include <QIcon>
#include "mainwindow.h"
#include "utils/tools.h"

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        writeToLogAndPrint("Debug: "+
                    QString(localMsg.constData())+" ("+
                    QString(context.file)+":"+
                    QString::number(context.line)+", " +
                    QString(context.function)+")", "nebula.log", 1);
        break;
    case QtWarningMsg:
        writeToLogAndPrint("Warning: "+
                           QString(localMsg.constData())+" ("+
                           QString(context.file)+":"+
                           QString::number(context.line)+", " +
                           QString(context.function)+")", "nebula.log", 1);
        break;
    case QtCriticalMsg:
        writeToLogAndPrint("Critical: "+
                           QString(localMsg.constData())+" ("+
                           QString(context.file)+":"+
                           QString::number(context.line)+", " +
                           QString(context.function)+")", "nebula.log", 1);
        break;
    case QtFatalMsg:
        writeToLogAndPrint("Fatal: "+
                           QString(localMsg.constData())+" ("+
                           QString(context.file)+":"+
                           QString::number(context.line)+", " +
                           QString(context.function)+")", "nebula.log", 1);
        abort();
    }
}


/* This is the top level GUI implementation */
int main(int argc, char **argv)
{
    // Initialize the log file
    QDateTime dateTime = dateTime.currentDateTime();
    QString dateTimeString = QString(dateTime.toString("dd/MM/yyyy hh:mm:ss"));
    writeToLogAndPrint("### NEBULA LOG "+dateTimeString+" ###", "nebula.log", 0);

    // Handle Qt messages
    qInstallMessageHandler(myMessageOutput);

    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/art/app.png"));

    app.setOrganizationName("Norwegian University of Science and Technology");
    app.setApplicationName("Nebula");

    MainWindow window;
    window.show();

    return app.exec();
}
